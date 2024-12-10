# Smart vehicle steering wheel lock system to prevent theft and DUIs
<p align="center">
  <img src="images/Device_demo.png" width="420" />
  <br>
  Device demo on steering wheel/dash
</p>

This Fourth-Year Capstone project at York University involved developing an innovative vehicle security system combining hardware design, embedded systems, and cloud computing. The core hardware component featured a custom PCB integrating an ESP32 microcontroller with GPS module and various sensors for comprehensive vehicle monitoring.
<p align="center">
  <img src="images/System architecture.png" width="420" />
  <br>
  System architecture broad overview
</p>

<div align="center">
<table>
  <tr>
    <td><img src="images/PCB_layer1.png" width="200">
    <p align="center">PCB layer 1</p></td>
    <td><img src="images/PCB_layer2.png" width="200">
    <p align="center">PCB layer 2</p></td>
    <td><img src="images/CustomPCB_withSensors.png" width="290">
    <p align="center">PCB with external sensors</p></td>
  </tr>
</table>
</div>

The embedded firmware was programmed in C++ to handle multiple tasks: sensor data collection, wireless communication, GPS tracking, and real-time data transmission. A key innovation was the cloud-based facial recognition system for driver authentication, developed using Python and Flask, containerized for deployment on Google Cloud Platform. The facial recognition service processes real-time video streams and maintains user authentication states. The system architecture implements Firebase real-time database enabling reliable communication between the microcontroller, cloud services, and mobile application.
<p align="center">
  <img src="images/Cloud Structure.png" width="800" />
  <br>
  Cloud Structure broad overview
</p>

The user interface features a custom Android application developed in Java, providing real-time monitoring capabilities, system control features, and secure user authentication.
<div align="center">
<table>
  <tr>
    <td><img src="images/Android_app_dashboard.png" width="250">
    <p align="center">Android App Dashboard</p></td>
    <td><img src="images/Android_app_GPSlocation.png" width="250">
    <p align="center">App GPS location view</p></td>
  </tr>
</table>
</div>
The entire system works cohesively through Firebase integration, creating a robust security solution that effectively prevents unauthorized vehicle access while maintaining user convenience and system reliability.


### Technologies: C++, Python, Java, Flask, Firebase, GCP, Android Studio, PCB Design, GPS
