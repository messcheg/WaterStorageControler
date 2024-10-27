We build a water storage using 9 IPC Containers that are connected  
to each otehr and in which we collect rainwater.                    
The containers are under the ground and we built a shed/poolhouse   
on top of it.                                                       
We have a hydrophore pump to use the water in the garden and we      
connected a valve to swhich between the pump and normal tapwater.   
This program is made for an Arduino to control the water system:    
- we use a pulse sensor to measure the water level in the containers
- to get a precice measuement, a thermometer measures the air       
  temprature in the containers.                                     
- there is a oled disply to display the waterlevel and other info   
- there is a (two phase) electronic switch valve, switching between 
  pump and tap water.                                               
- there are two ralais to set the valve to one or the other         
  direction                                                         
                                                                    
The process works as follows:                                       
- periodically the arduino measures the water level                 
- if the level is below a certain treshold, the system will switch  
  to tapwater                                                       
- if the level is above a certain level, the system will switch to  
  pumpwater      
- you can find the scheme on https://app.cirkitdesigner.com/project/c0ec4d54-bd06-43c7-bd29-d0260e1bef4a                                                   
