# Raingarden Low-cost Stormwater Sensor
Stormwater flow sensor for raingardens
Over 800 communities around the U.S. collect their stormwater along with sanitary and industrial wastewater in combined sewer systems. In these communities, excessive rainfall or snowmelt may result in combined sewer overflows (CSO) that, according to the Environmental Protection Agency (EPA), affect approximately 40 million people in 32 states every year. While current environmental regulations impede the construction of new combined sewers, mandating separate sanitary and stormwater systems, the legacy infrastructure of combined sewers still experiences thousands of overflows per year. Furthermore, large wet weather events may also produce stormwater runoff where water runs over impervious surfaces collecting contaminants and debris on its way to the sewer. The combined effect of large impervious areas such as parking lots, large roofs or paved streets may result in considerable amounts of stormwater runoff that ultimately contaminates a body of water. To mitigate the problem communities around the U.S. are employing raingardes to return the stormwater to the ground in the place where it is collected. 

Over the past two years we have worked on developing low-cost, stormwater sensor. The sensor is based on Particle's Electron device and a water flow sensor based on a pendulum design. Attached to the pendulum there is a MPU6050 based accelerometer that allows the sensor to collect angle readings and then publish them to ATT's M2X via Particle's Webhooks. We have tested our sensor at two locations. The first one at the Athens City Community Center raingarden and the second one at the Athens City Code Enforcement office. More details can be found at www.its.ohio.edu/raingardens.

To make it easier to replicat our design we employed OnShape's public CAD system to design the 3D printed parts necessary to house the sensor. There are two designs we have made public via OnShape. One for a sensor that can be installed on a PVC pipe and a second for a sensor that can be mounted on the exterior of a 6 inch PVC 90 degree fitting.

The sensors housings we designed and mounted are shown below. A 3D detail is also available in the main branch.

![CeO Install](https://raw.githubusercontent.com/OUSmartInfrastructure/raingardenSensor/master/athensCeOInstall.jpg) 
![Community Center Install](https://raw.githubusercontent.com/OUSmartInfrastructure/raingardenSensor/master/athensCommCenterInstall.png) 

## Getting started
To get things started, you will need:
 - [Particle Electron](https://www.particle.io/products/hardware)
 - An [MPU 6050](https://www.sparkfun.com/products/11028) sensor board or other I2C based accelerometer
 - A 10W 12V [solar panel](https://www.amazon.com/ALEKO%C2%AE-10W-10-Watt-%20Monocrystalline-Solar/dp/B00G4KHJN6)
 - A solar charge [controller](https://www.amazon.com/Sunix-Controller-Intelligent-Temperature-Compensation/dp/B01J5A6X92/ref=pd_sbs_86_2?_encoding=UTF8&refRID=B8DY326YZ77B3RJT9QPY&th=1)
 - 3D Print one of the sensor housings from Onshape: [Design 1](https://cad.onshape.com/documents/c7ef5115150ea284fa6ded5d/w/f29a5a49622384c412d514d9/e/a90ee472d2e98eb237201269), [Design 2](
https://cad.onshape.com/documents/ec36e10f84544662c55e4cf2/w/c337c54d4c1919a4cff84898/e/298fa85778e384ae9d079753)
 - A outdoor [utility box](https://www.amazon.com/Wise-%20Outdoors-5601-13-Small-%20Utility/dp/B0019JXDQE/)
 - A [terminal board](https://store.ncd.io/product/screw-terminal-breakout-board-for-particle-photon-or-particle-electron/) for the Electron to keep your connections in place
 - A 15,000 mAh acid-lead battery
 - Various cables and connectors
