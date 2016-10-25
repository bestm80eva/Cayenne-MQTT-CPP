/**
* @file CayenneClient.cpp
*
* Example app for using the Cayenne MQTT C++ library to send and receive example data.
*/

#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include "CayenneMQTTClient.h"
#include "MQTTLinux.h"

// Cayenne authentication info. This should be obtained from the Cayenne Dashboard.
char username[] = "MQTT_USERNAME";
char clientID[] = "CLIENT_ID";
char password[] = "MQTT_PASSWORD";

MQTTNetwork ipstack;
Cayenne::MQTTClient<MQTTNetwork, MQTTTimer> mqttClient(ipstack);
bool finished = false;


/**
* Print the message info.
* @param[in] message The message received from the Cayenne server.
*/
void outputMessage(Cayenne::MessageData& message)
{
	switch (message.topic)	{
	case COMMAND_TOPIC:
		printf("topic=Command");
		break;
	case CONFIG_TOPIC:
		printf("topic=Config");
		break;
	default:
		printf("topic=%d", message.topic);
		break;
	}
	printf(" channel=%d", message.channel);
	if (message.clientID) {
		printf(" clientID=%s", message.clientID);
	}
	if (message.type) {
		printf(" type=%s", message.type);
	}
	for (size_t i = 0; i < message.valueCount; ++i) {
		if (message.values[i].value) {
			printf(" value=%s", message.values[i].value);
		}
		if (message.values[i].unit) {
			printf(" unit=%s", message.values[i].unit);
		}
	}
	if (message.id) {
		printf(" id=%s", message.id);
	}
	printf("\n");
}

/**
* Handle messages received from the Cayenne server.
* @param[in] message The message received from the Cayenne server.
*/
void messageArrived(Cayenne::MessageData& message)
{
	int error = 0;
	// Add code to process the message. Here we just ouput the message data.
	outputMessage(message);

	// If this is a command message we publish a response. Here we are just sending a default 'OK' response.
	// An error response should be sent if there are issues processing the message.
	if (message.topic == COMMAND_TOPIC && (error = mqttClient.publishResponse(message.channel, message.id, NULL, message.clientID)) != CAYENNE_SUCCESS) {
		printf("Response failure, error: %d\n", error);
	}
}

/**
* Connect to the Cayenne server.
* @return Returns CAYENNE_SUCCESS if the connection succeeds, or an error code otherwise.
*/
int connectClient(void)
{
	int error = 0;
	// Connect to the server.
	printf("Connecting to %s:%d\n", CAYENNE_DOMAIN, CAYENNE_PORT);
	while ((error = ipstack.connect(CAYENNE_DOMAIN, CAYENNE_PORT)) != 0) {
		if (finished)
			return error;
		printf("TCP connect failed, error: %d\n", error);
		sleep(2);
	}

	if ((error = mqttClient.connect(username, clientID, password)) != MQTT::SUCCESS) {
		printf("MQTT connect failed, error: %d\n", error);
		return error;
	}
	printf("Connected\n");

	// Subscribe to required topics.
	if ((error = mqttClient.subscribe(COMMAND_TOPIC, CAYENNE_ALL_CHANNELS)) != CAYENNE_SUCCESS) {
		printf("Subscription to Command topic failed, error: %d\n", error);
	}
	if ((error = mqttClient.subscribe(CONFIG_TOPIC, CAYENNE_ALL_CHANNELS)) != CAYENNE_SUCCESS) {
		printf("Subscription to Config topic failed, error:%d\n", error);
	}

	// Send device info. Here we just send some example values for the system info. These should be changed to use actual system data, or removed if not needed.
	mqttClient.publishData(SYS_VERSION_TOPIC, CAYENNE_NO_CHANNEL, NULL, NULL, CAYENNE_VERSION);
	mqttClient.publishData(SYS_MODEL_TOPIC, CAYENNE_NO_CHANNEL, NULL, NULL, "Linux");
	mqttClient.publishData(SYS_CPU_MODEL_TOPIC, CAYENNE_NO_CHANNEL, NULL, NULL, "CPU Model");
	mqttClient.publishData(SYS_CPU_SPEED_TOPIC, CAYENNE_NO_CHANNEL, NULL, NULL, "1000000000");

	return CAYENNE_SUCCESS;
}

/**
* Main loop where MQTT code is run.
*/
void loop(void)
{
	MQTTTimer timer(5000);

	while (!finished) {
		// Yield to allow MQTT message processing.
		mqttClient.yield(1000);

		// Check that we are still connected, if not, reconnect.
		if (!ipstack.connected() || !mqttClient.connected()) {
			ipstack.disconnect();
			mqttClient.disconnect();
			printf("Reconnecting\n");
			while (connectClient() != CAYENNE_SUCCESS) {
				if (finished)
					return;
				sleep(2);
				printf("Reconnect failed, retrying\n");
			}
		}

		// Publish some example data every few seconds. This should be changed to send your actual data to Cayenne.
		if (timer.expired()) {
			int error = 0;
			if ((error = mqttClient.publishData(DATA_TOPIC, 0, TEMPERATURE, CELSIUS, 30.5)) != CAYENNE_SUCCESS) {
				printf("Publish temperature failed, error: %d\n", error);
			}
			if ((error = mqttClient.publishData(DATA_TOPIC, 1, LUMINOSITY, LUX, 1000)) != CAYENNE_SUCCESS) {
				printf("Publish luminosity failed, error: %d\n", error);
			}
			if ((error = mqttClient.publishData(DATA_TOPIC, 2, BAROMETRIC_PRESSURE, HECTOPASCAL, 800)) != CAYENNE_SUCCESS) {
				printf("Publish barometric pressure failed, error: %d\n", error);
			}
			timer.countdown_ms(5000);
		}
	}
}

/**
* Interrupt handler for processing program shutdown.
*/
void intHandler(int signum) {
	finished = true;
}

/**
* Main function.
* @param[in] argc Count of command line arguments.
* @param[in] argv Command line argument string array.
*/
int main(int argc, char** argv)
{
	// Set up handler so we can exit cleanly when Ctrl-C is pressed.
	struct sigaction action;
	memset(&action, 0, sizeof(struct sigaction));
	action.sa_handler = intHandler;
	sigaction(SIGINT, &action, NULL);
	// Ignore the SIGPIPE signal since the program will attempt to reconnect if there are socket errors.
	action.sa_handler = SIG_IGN;
	sigaction(SIGPIPE, &action, NULL);

	// Set the default function that receives Cayenne messages.
	mqttClient.setDefaultMessageHandler(messageArrived);

	// Connect to Cayenne.
	if (connectClient() == CAYENNE_SUCCESS) {
		// Run main loop.
		loop();
	}
	else {
		printf("Connection failed, exiting\n");
	}

	if (mqttClient.connected())
		mqttClient.disconnect();
	if (ipstack.connected())
		ipstack.disconnect();

	return 0;
}

