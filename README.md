# *Dusty miner*

### Contributors <!-- omit in toc -->
- [Alberto Huertas](https://github.com/aHuertasR)
- [Ramón Hernandez](https://github.com/uRHL)

### Sections <!-- omit in toc -->

- [*Dusty miner*](#dusty-miner)
	- [Objective](#objective)
	- [Materials](#materials)
	- [Functional specifications](#functional-specifications)
		- [Speed control](#speed-control)
		- [Concrete mixer control](#concrete-mixer-control)
		- [Lamp control](#lamp-control)
		- [I/O control](#io-control)
		- [Loading point control](#loading-point-control)
		- [Emergency mode control](#emergency-mode-control)
	- [Sensors](#sensors)
	- [Actuators](#actuators)
	- [Execution modes - Software module](#execution-modes---software-module)
			- [Normal mode](#normal-mode)
			- [Braking mode](#braking-mode)
			- [Stop mode (SW)](#stop-mode-sw)
			- [Emergency mode (SW)](#emergency-mode-sw)
	- [Execution modes - Hardware module](#execution-modes---hardware-module)
			- [Distance selection mode](#distance-selection-mode)
			- [Approach mode](#approach-mode)
			- [Stop mode (HW)](#stop-mode-hw)
			- [Emergency mode (HW)](#emergency-mode-hw)
	- [Real-time formalization](#real-time-formalization)
		- [Software module](#software-module)
		- [Hardware module](#hardware-module)

---

## Objective

Build a real time system for a wagon so that it can carry safely concrete through a railway. It must be able to:

- maintain the speed within the safe range.
- mix the concrete so that it is not solidified.
- turn on/off the lamps during dark zones (tunnels)
- stop the wagon in the corresponding loading points (deposits)
- stop the wagon on emergencies.

## Materials

Software: Raspberry Pi (RTMS OS)

Hardware: Arduino moderboard

## Functional specifications

### Speed control

The environment (railway) modifies the speed of the wagon
* Flat slope: do not affect the speed
* Up slope: Constant speed reduction (-0.25 m/s2)
* Down slope: Constant acceleration (0.25 m/s2)

### Concrete mixer control

- If the concrete is not mixed for 60 seconds it starts to solidify.
- Additionally, the concrete requires to be mixed for at least 30 seconds.
- If the state of the mixer had not change in the last 30 seconds modify the state of the mixer.
- The mixer is overcharged (broken) if it is working for more than 60 seconds consecutively.
- The mixer requires 30 seconds to rest between each swith (on/off)

### Lamp control

- The lamps should be turned on/off in less that 12 seconds after getting in/out of the tunnel

### I/O control

- Messaging the hardware module and wait for the answer (400 ms)
- Show the new state on the display (display functions) (500 ms)
- Total cycle duration is 900 ms

### Loading point control

### Emergency mode control


## Sensors
- **Speed sensor**: returns the current speed of the wagon
- **Slope sensor**: returns the current slope
- **Light sensor**: detects the intensity of the environmental light and indicates if the lights should be turned on or not
- **Distance sensor**: returns the remaining distance between the current point and the next station
- **Loading sensor**: detects if the wagon is still being loaded (the wagon cannot move) or if it has finished (the wagon can continue)


## Actuators
- **Accelerator (on/off)**: Applies a constant acceleration (0.5 m/s2)
- **Brake (on/off)**: Applies a constant speed reduction (-0.5 m/s2)
- **Mixer (on/off)**: turns on/off the mixer's engine
- **Lamps (on/off)**

---

## Execution modes - Software module

#### Normal mode

- Safe speed range: 40m/s – 70m/s
- The distance until the next stop is checked periodically. When the *safe braking distance* is reached, the wagon enters in *braking mode* (normal mode -> braking mode)
- *Safe braking distance*: minimum distance required to stop safely the wagon in the next station. Consider always the worst case scenario

#### Braking mode

- Wagon speed < 10 m/s
- Lamps always on
- The tasks speed, acceleration and brake control have their period reduced to the half they have while in normal mode
- When the distance until the station is 0 meters, and the rest of requirements are met, the wagon enters in stop mode (braking mode -> stop mode)

#### Stop mode (SW)

- The wagon is stopped (speed 0 m/s) until the *loading sensor* indicates that the loading process has finished (stop mode -> normal mode)
- Lamps always on
- The mixer should keep working

#### Emergency mode (SW)

Whenever there is a failure (e.g. timing failure), regardless of the operation mode, the wagon enters in emergency mode. When this happens:

- The software module notifies the hardware module to enter in emergency mode
- The wagon is stopped completely (0 m/s)
- The lamps are always turned on
- The mixer keeps working


## Execution modes - Hardware module

#### Distance selection mode

- The wagon periodicaly updates the distance to the next station
- The wagon compares the current distance to the next station and the *safe braking distance* to decide if the operation mode should change from *normal* to *approaching*.
- The display shows the distance to the next station 

#### Approach mode

- The wagon compares the current distance to the next station. When it reaches 0, the wagon transitions from *approaching mode* to *stop mode*.
- The display shows the distance remaining until the station

#### Stop mode (HW)

- The displays shows the distance until the next station (0 meters)
- Checks the value of the *loading sensor* to decide if the operation mode should change from *loading* to *normal*.

#### Emergency mode (HW)
The harware module will switch to emergency mode when the sofware
module request it.

---


## Real-time formalization

#### Symbol table <!-- omit in toc -->

| Symbol | Meaning |
|:---:|:---:|
| (\*) | Recommended value, but it can be modified |
| X | Task not executed |
| T/D | Task period or deadline |
| C | Task execution duration |

### Software module

Notes

- Time is measured in seconds
- Ci = 0.9 for all tasks

|  | Normal mode | Braking mode | stop mode | Emergency mode |
|:---:|:---:|:---:|:---:|:---:|
|  |  |  |  |  |
| Task | T/D | T/D | T/D | T/D |
| Read slope | 10 | 10(\*) | X | 10 |
| Read speed | 10 | 5 | X | 10 |
| Read distance | 10 | 10(\*) | X | X |
| Turn on/off accelerator | 10 | 5 | X | 10 |
| Turn on/off brake | 10 | 5 | X | 10 |
| Turn on/off mixer | 15 | 15 | 15 | 15 |
| Read light sensor | 6 | X | X | X |
| Turn on/off lamps | 6 | 30(\*) | 5(\*) | 6 |
| Read loading sensor | X | X | 5(\*) | 10(\*) |
| Activate the emergency mode (no return) | X | X | X | 10(\*) |


### Hardware module

Notes

- Time is measured in ms
- Once the Arduino circuit is built, the real duration of each task must be measured manually several times. The final value used for Ci will be the worst one among all the measures. 

|  | Distance selection mode | Approaching mode | stop mode | Emergency mode |
|:----:|:----:|:----:|:----:|:----:|
|  |  |  |  |  |
| Task T/D C | T/D | T/D | T/D | T/D |
| On/Off accelerator | 200(\*) | 200(\*) | 200(\*) | 200(\*) |
| On/Off brake | 200(\*) | 200(\*) | 200(\*) | 200(\*) |
| On/Off Mixer | 200(\*) | 200(\*) | 200(\*) | 200(\*) |
| Compute and show speed | 200(\*) | 200(\*) | 200(\*) | 200(\*) |
| Read slope | 200(\*) | 200(\*) | 200(\*) | 200(\*) |
| Read light sensor | 200(\*) | 200(\*) | 200(\*) | X |
| Turn on/off lamps | 200(\*) | 200(\*) | 200(\*) | 200(\*) |
| Distance selection | 200(\*) | X | X | X |
| Display distance | 200(\*) | 200(\*) | X | X |
| Validate distance | 200(\*) | X | X | X |
| Read end of slope | X | X | 200(\*) | X |

