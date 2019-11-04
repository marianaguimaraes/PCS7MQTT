# PCS7 Arduino and MQTT

Project to communicate an Arduino to Siemens PCS7 PLC and publish/subcribe values by MQTT protocol

Using lib PubSubClient and Settimino, but Settimino with changes - in repository

Set the IP's

Check cpu slot number

Set your MQTT server, login and password (default port 1883)

I create 2 topics on my MQTT server, one by recive TopicIn and other to send TopicOut

We are reading a REAL db and write a bool db.
