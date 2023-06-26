# Hardware Requirements for Connectivity

Before Aqualink Automate can connect to an RS Aqualink control panel, a physical connection needs to be created between the panel itself and the server running the Aqualink Automate software.

## Serial Connectivity

There are several options ranging from cheap to expensive to connect to the RS Aqualink panel.  The most basic setup uses a USB-2-RS485 adapter connected to both the server (via USB) and to the RS Aqualink panel (via serial).  For more remote installations, or installations that want a larger physical distance between the panel and the server, a serial-over-ethernet solution can be used.

### Connecting a USB-2-RS485 Adapter

TBC

### Connecting a Serial-over-Ethernet Adapter

TBC

### Serial Connection Guide (aka the wires)

1. **Identifying Connections:** The simplest configuration is to connect the Data+ and Data- from an USB-2-RS485 adapter to the Data+ and Data- lines of the RS-485 interface of the RS Aqualink panel.
2. **Cabling:** Standard RS485 pool equipment cable includes 4 wires - Data+, Data-, Power+, and Ground-. Wire colors may vary depending on your installation, and labeling has evolved over the years. Hence, we recommend consulting your pool equipment manual for accurate information.
3. **Connecting to the Control Panel:** The control panel usually has a 4-post connector at the top right (some might have 2). This is the RS485 interface. Although the wire colors will be labeled, they won't indicate their functionality. The most common configuration is Green, Yellow, Black, Red.

Note that some USB adapters might have 3, 4, or even 6 connections. The third connection is typically used for ground (which can be crucial or problematic depending on your specific setup), and the fourth one is typically used for power and should not be used with USB-2-RS485 adapters.

RS Aqualink wiring most commonly uses the following colour configuration: Green, Yellow, Black, Red.  These wires will found at the top of the RS Aqualink panel.  When following the standard colour configuration, the following table shows the typical installation:

| Wire Colour  | USB-2-RS485 Pin | Description | Function                  |
|--------------|-----------------|-------------|---------------------------|
| Black        | Pin A           | Data+       | Data signaling line (+ve) |
| Yellow/White | Pin B           | Data-       | Data signaling line (-ve) |
| Green        | GND             | Ground      | Common/shared ground line |
| Red          | Not Used        | Power       | Not Used                  |

Note that the ground may be considered optional as RS-485 is a differential signal i.e. no common ground is required however adding that third wire between the server and panel may be done to limit the common mode voltage and help with noise supression.

### Wiring and Cabling Recommendations

The wiring and connections to the adapter and panel are critical to getting a good signal. Many issues with poor message checksums i.e. lots of rejected messages, usually occur due to wiring problems.  The recommended wire is 24AWG twisted pair; below is a paragraph taken from [RS Cable Selection](http://www.bb-elec.com/Learning-Center/All-White-Papers/Serial/Cable-Selection-for-RS422-and-RS485-Systems/Cable-Selection-for-RS-422-and-RS-485-Systems.PDF)

> The RS-422 specification recommends 24AWG twisted pair cable with a shunt capacitance of 16 pF per foot and 100 ohm characteristic impedance. While the RS-485 specification does not specify cabling, these recommendations should be used for RS-485 systems as well.
