I decided on the LIFX platform for my home smart lighting system, as it works directly over wifi.
I saw it even advertised an API, though most people use the cloud api for controlling lights as it is much more simple
However, LIFX offer no battery powered options for lighting control (likely due to the heavy weight nature of a wifi implementation).
Enter SwitchyFX!


This implementation is a learning project for C++, I cannot make any guarentee's it works, but if it does it should be a lot faster then a cloud server round trip.
Hopefully soon when I have some spare time I will implement the LowPower state options available with the ESP8266 to extend the battery life from a few hours to a few months.
