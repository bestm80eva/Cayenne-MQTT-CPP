/**
* @file SimpleSubscribe.cpp
*
* Simplified example app for using the Cayenne MQTT C++ library to receive example data.
*/

#include "MQTTLinux.h"
#include "CayenneMQTTClient.h"

// Cayenne authentication info. This should be obtained from the Cayenne Dashboard.
char username[] = "MQTT_USERNAME";
char clientID[] = "CLIENT_ID";
char password[] = "MQTT_PASSWORD";

MQTTNetwork ipstack;
Cayenne::MQTTClient<MQTTNetwork, MQTTTimer> mqttClient(ipstack);


// Handle messages received from the Cayenne server.
void messageArrived(Cayenne::MessageData& message)
{
	printf("Message received on channel %d\n", message.channel);
	
	// Add code to process the message here.

	// If this is a command message we publish a response. Here we are just sending a default 'OK' response.
	// An error response should be sent if there are issues processing the message.
	if (message.topic == COMMAND_TOPIC) {
		mqttClient.publishResponse(message.channel, message.id, NULL, message.clientID);
	}
}

// Connect to the Cayenne server.
int connectClient(void)
{
	// Connect to the server.
	int error = 0;
	printf("Connecting to %s:%d\n", CAYENNE_DOMAIN, CAYENNE_PORT);
	if ((error = ipstack.connect(CAYENNE_DOMAIN, CAYENNE_PORT)) != 0) {
		return error;
	}

	if ((error = mqttClient.connect(username, clientID, password)) != MQTT::SUCCESS) {
		ipstack.disconnect();
		return error;
	}
	printf("Connected\n");

	// Subscribe to required topics. Here we subscribe to the Command and Config topics.
	mqttClient.subscribe(COMMAND_TOPIC, CAYENNE_ALL_CHANNELS);
	mqttClient.subscribe(CONFIG_TOPIC, CAYENNE_ALL_CHANNELS);

	return CAYENNE_SUCCESS;
}


// Main loop where MQTT code is run.
void loop(void)
{
	while (1) {
		// Yield to allow MQTT message processing.
		mqttClient.yield(1000);
	}
}

// Main function.
int main(int argc, char** argv)
{
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

	return 0;
}

