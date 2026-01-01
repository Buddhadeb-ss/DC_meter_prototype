# DC_meter_prototype
What is this project?
This is a Smart Power Meter I built to measure DC power (Direct Current). Most energy meters in our houses only measure AC electricity (the wall socket), but they are useless for things like batteries, solar panels, or electronics projects.

My device measures Voltage, Current, and Power in real-time and shows the data on a small screen and sends it to a website over Wi-Fi.

🧩 The Parts I Used
Brain: ESP32 (It has Wi-Fi built-in).
Sensor: INA219 (A specialized chip that measures voltage and current).
Display: 0.96-inch OLED Screen (To see stats without a phone).
The Test Load: A 9V Battery and a DC Motor.

⚙️ How It Works (The Logic)
Reading the Data: The INA219 sensor sits between the battery and the motor. It constantly checks how much power acts on the motor.
Processing: The ESP32 reads this data 10 times every second.
Multitasking: I programmed the ESP32 to do two things at once (Asynchronous programming):
Update the OLED screen instantly so it doesn't lag.
Upload the data to the cloud every few seconds so I can view it on my laptop
UPDATE1: Now the device can send data online and this can be accessed by Blynk Application

SCHEMATIC-
<img width="975" height="462" alt="image" src="https://github.com/user-attachments/assets/2c73c0e5-b6f9-45d0-b441-1654bb756c7c" />
BLOCK DIAGRAM-
<img width="1017" height="539" alt="image" src="https://github.com/user-attachments/assets/1357fdde-7ee7-4d09-b1fb-3827f9c1ac58" />
RESULTS-
<img width="450" height="450" alt="image" src="https://github.com/user-attachments/assets/a166d46a-19ea-44b7-aff2-dacc97c86c30" />
PROTOTYPE-
<img width="1280" height="960" alt="image" src="https://github.com/user-attachments/assets/16df1f6b-3db1-4929-9471-b5f7511f9030" />



