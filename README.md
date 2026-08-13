# ESP32 Wake-on-LAN over MQTT & Remote Access

Remotely wake your PC by publishing an MQTT message to an ESP32. The ESP32 receives the command and sends a Wake-on-LAN (WoL) magic packet over your local network.

The project can be used as a **standalone MQTT-based Wake-on-LAN solution**, or extended with additional remote-access tools such as Tailscale and Moonlight/Sunshine to provide full remote PC access.

### Wake-on-LAN

```text
Phone / Laptop
      │
      │ MQTT
      ▼
 HiveMQ Cloud
      │
      │ MQTT over TLS
      ▼
    ESP32
      │
      │ WoL magic packet
      ▼
     PC
```

The ESP32 acts as a small, always-available bridge between an internet-accessible MQTT broker and the PC's local Wake-on-LAN interface. This allows the PC to be powered on remotely without requiring the PC itself to be online.

### Remote Access

Once the PC is awake, you can optionally use a remote-access solution such as Tailscale together with Sunshine/Moonlight to connect to and control the PC remotely.

```text
┌─── MQTT ───► ESP32 ───► WoL ───► PC
│
Laptop
│
└── Tailscale ───────────────────► PC
│
│ Sunshine
▼
Moonlight
```

You can therefore use only the **WoL portion** if all you need is remote power-on, or use the complete setup for **remote PC access** after the machine has been awakened.

## Remote operation

### Remote wake-up

These are required to remotely turn on the PC:

- An MQTT broker accessible from the ESP32 and remote client
- An MQTT client capable of publishing the `WAKE` command
- The ESP32 must remain powered and connected to the local network

The MQTT portion does not require the ESP32 to have a public IP address. CGNAT generally does not prevent the ESP32 from maintaining its outbound MQTT connection to the broker.

### Remote PC access

These are used to access the PC after it has been powered on:

- **Tailscale** is recommended for private remote access, especially when the home network uses CGNAT
- A Tailscale client installed on both the remote device and the PC
- **Moonlight** and **Sunshine** if you want to remotely stream the PC's desktop or games

Tailscale is **not required for the ESP32 wake-up functionality**. It is used for communicating with the PC after it has booted.

# How it works

- The ESP32 connects to your 2.4 GHz Wi-Fi network.
- It connects to an MQTT broker using MQTT over TLS.
- It subscribes to a topic, `home/pc/wake` by default.
- When it receives the payload `WAKE` on that topic, it sends a Wake-on-LAN magic packet to the configured PC.
- A 10-second cooldown prevents repeated `WAKE` messages from causing multiple WOL packets in quick succession.
- If the Wi-Fi connection is lost, the ESP32 attempts to reconnect.
- If the MQTT connection is lost, the ESP32 periodically attempts to reconnect.

## Why MQTT is useful for remote WOL

Some home Internet provider uses **CGNAT (Carrier-Grade NAT)**.

### CGNAT

CGNAT can make directly reaching your home PC from the Internet difficult.

With CGNAT, your router usually does not have a publicly reachable IPv4 address. As a result, directly connecting to the ESP32 from the Internet with port forwarding may not be possible, even if port forwarding is configured correctly on the router. Instead, the ESP32 makes an **outbound connection** to the MQTT broker.

Because the ESP32 initiates the connection to HiveMQ, CGNAT generally does not prevent this connection. The MQTT broker acts as the intermediary between the remote publisher and the ESP32.

Tailscale is designed to work around this by establishing connections between devices through NAT traversal and, when necessary, relay servers.

### Tailscale

Tailscale provides private remote access to the PC after it has booted, but it is not required for the ESP32 wake-up path. A powered-off PC cannot participate in the Tailscale network because its operating system and Tailscale client are not running. The ESP32 remains powered and connected to the local network, so it acts as the always-on bridge that receives the MQTT `WAKE` command and sends the Wake-on-LAN magic packet on the local LAN. In short:

- **MQTT + ESP32**: handles remotely triggering the PC to power on.
- **Tailscale**: provides private connectivity to the PC once it is online (post-boot).

Tailscale can be useful to access services such as Sunshine or SSH after the PC starts.

Install [Tailscale](https://tailscale.com/download) on:

- The host PC.
- The client machine.

Therefore, a common setup for this project is:

```text
                    Remote Device
                         │
                         │
                         ▼
                        MQTT
                         │
                         │
                         ▼
                      ESP32
                         │
                         │ WOL
                         ▼
                      Gaming PC
                         │
                      Tailscale
                         │
                    Remote device
```

| Technology             | Purpose                                                                |
| ---------------------- | ---------------------------------------------------------------------- |
| **MQTT**               | Remotely tell the ESP32 to wake the PC                                 |
| **ESP32**              | Remains available and sends WOL on the local LAN                       |
| **Wake-on-LAN**        | Sends a magic packet to wake the PC                                    |
| **Tailscale**          | Provides private remote connectivity to the PC after it is online      |
| **CGNAT**              | The reason direct inbound connections to the home network may not work |
| **Sunshine/Moonlight** | Streams the PC to the remote client                                    |

You therefore do not need to expose the ESP32, PC, SSH, or an MQTT broker hosted on your home network directly to the Internet.

# Requirements

## Hardware

- Any ESP32 development board with 2.4 GHz Wi-Fi
- A PC with a network adapter and motherboard/firmware that support Wake-on-LAN (WOL)
- Ethernet connection on the PC is strongly recommended for reliable WOL
- The ESP32 should normally be connected to the same LAN/subnet as the PC.

  > Advanced network configurations may allow WOL across subnets, but this is not covered by this project.

- The ESP32 must remain powered and connected to the local network while the PC is shut down. For remote wake-up over MQTT, the ESP32 also needs Internet access so it can connect to the MQTT broker.

## Software

- [Arduino IDE](https://www.arduino.cc/en/software) or PlatformIO
- ESP32 board support package
  - **Arduino IDE:** Add the following URL to **File → Preferences → Additional Boards Manager URLs**:
    `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`
  - Then open **Tools → Board → Boards Manager**, search for **esp32**, and install **esp32 by Espressif Systems**.
  - **PlatformIO:** ESP32 support is managed through the PlatformIO platform configuration, so the additional Boards Manager URL is not required.
- `PubSubClient`
- `WiFiClientSecure`, `WiFiUdp`, and `WiFi` from the ESP32 Arduino core
- [Mosquitto](https://mosquitto.org/download/) for publishing MQTT commands

This project uses [HiveMQ Cloud](https://www.hivemq.com/mqtt-cloud-broker/) as the example MQTT broker, but other MQTT brokers supporting MQTT over TLS can be used.

# 1. Set up an MQTT broker

This project uses HiveMQ Cloud as the example broker.

1. Go to [HiveMQ Cloud](https://console.hivemq.cloud/) and create an account.
2. Create a cluster.
3. Once the cluster has been created, find the cluster URL.

It will look similar to:

```text
xxxxxxxxxx.s1.eu.hivemq.cloud
```

This becomes:

```cpp
MQTT_SERVER
```

The TLS MQTT port is:

```text
8883
```

4. Create MQTT credentials under the cluster's access management section.
5. Record the username and password.
6. These become:

```cpp
MQTT_USERNAME
MQTT_PASSWORD
```

For better security, restrict the MQTT credentials to only the topics and actions the device needs.

# 2. Install the CA Certificate

The Windows Mosquitto client uses TLS to securely connect to the MQTT broker. For the HiveMQ Cloud configuration used in this guide, Mosquitto can use the **ISRG Root X1 CA certificate** to verify the broker's certificate chain.

### Download the certificate

[Download the `isrgrootx1.pem` certificate provided for HiveMQ Cloud](https://letsencrypt.org/certs/isrgrootx1.pem).

Save the file somewhere on your computer. For example:

```text
C:\Users\YourUsername\isrgrootx1.pem
```

# 3. Configure the PC for Wake-on-LAN

Wake-on-LAN needs to be enabled on the target PC before the ESP32 can wake it.

## BIOS / UEFI

The exact option name varies by motherboard.

Look for settings such as:

- Wake on LAN
- Wake on PCI-E
- Power On By PCI-E
- Resume by LAN

Enable the appropriate option.

## Windows

Open:

**Device Manager → Network adapters → Your Ethernet adapter → Properties**

Under **Power Management**, enable:

> Allow this device to wake the computer

Under **Advanced**, look for an option such as:

> Wake on Magic Packet

Set it to:

> Enabled

### Windows Fast Startup

Fast Startup can interfere with Wake-on-LAN after a Windows shutdown on some systems.

If WOL works from sleep but not from a full shutdown, try disabling Windows Fast Startup.

# 4. Find your PC's MAC address

The ESP32 needs the MAC address of the network adapter that receives the WOL packet.

On Windows, open Command Prompt:

```cmd
ipconfig /all
```

Find the active Ethernet adapter and look for:

```text
Physical Address
```

For example:

```text
04-7C-16-3E-A4-87
```

The MAC address is written in hexadecimal.

In the Arduino sketch, convert it to six hexadecimal byte values:

```cpp
byte PC_MAC[] = {
  0x04, 0x7C, 0x16, 0x3E, 0xA4, 0x87
};
```

The `0x` prefix tells C++ that each value is hexadecimal.

The following are equivalent representations of the same MAC address:

```text
04-7C-16-3E-A4-87
```

and:

```cpp
0x04, 0x7C, 0x16, 0x3E, 0xA4, 0x87
```

# 5. Find your LAN broadcast address

The ESP32 sends the WOL magic packet to the configured LAN broadcast address.

The correct **subnet-directed broadcast address** depends on your local IP address and subnet mask

For example, if a device on your network has:

```text
IP address:    192.168.12.100
Subnet mask:   255.255.255.0
```

then the broadcast address is:

```text
192.168.12.255
```

For example, this project used:

```text
ESP32 IP:      192.168.8.182
Subnet:        255.255.255.0
Broadcast:     192.168.8.255
```

Another network might use:

```text
192.168.1.x
```

with:

```text
255.255.255.0
```

in which case the broadcast address would normally be:

```text
192.168.1.255
```

# 6. Configure the Arduino project

The project uses a `secrets.h` file to keep private credentials out of the main sketch along with configurations.

Put private network and MQTT credentials in it:

```cpp
// ============ SECRETS ============

const char* WIFI_SSID = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";

const char* MQTT_USERNAME = "YOUR_MQTT_USERNAME";
const char* MQTT_PASSWORD = "YOUR_MQTT_PASSWORD";

const char* MQTT_SERVER = "YOUR_MQTT_SERVER_ADDRESS";
const int MQTT_PORT = 8883;

// ============ PC CONFIGURATION ============

// Get this from `ipconfig /all`.
//
// Example:
// A6-3D-91-7B-42-E8
//
// Enter it as six hexadecimal bytes.
byte PC_MAC[] = {
  0xA6, 0x3D, 0x91, 0x7B, 0x42, 0xE8
};

// Find this from your local IP address and subnet mask.
//
// Example:
// 192.168.8.x + 255.255.255.0
// = 192.168.8.255
IPAddress BROADCAST_IP(192, 168, 8, 255);
```

### Configuration reference

| Variable        | Description                                 |
| --------------- | ------------------------------------------- |
| `WIFI_SSID`     | Your Wi-Fi network name                     |
| `WIFI_PASSWORD` | Your Wi-Fi password                         |
| `MQTT_USERNAME` | MQTT broker username                        |
| `MQTT_PASSWORD` | MQTT broker password                        |
| `MQTT_SERVER`   | MQTT broker hostname                        |
| `MQTT_PORT`     | MQTT over TLS port used by your broker      |
| `PC_MAC`        | MAC address of the PC's WOL network adapter |
| `BROADCAST_IP`  | Broadcast address of your local network     |

# 7. Configure the MQTT topic

The default MQTT topic is:

```cpp
const char* MQTT_TOPIC = "home/pc/wake";
```

When the ESP32 receives:

```text
WAKE
```

on that topic, it sends the WOL packet.

You can change the topic if desired.

For example:

```cpp
const char* MQTT_TOPIC = "myhome/computer/wake";
```

If you change it in the ESP32 sketch, make sure your MQTT clients publish to the same topic.

# 8. Flash the ESP32

1. Open the `.ino` file in Arduino IDE.
2. Select your ESP32 board under:
   **Tools → Board → esp32**
3. Select the correct serial port.
4. Click **Upload**.
5. Open the Serial Monitor.
6. Set the baud rate to:

```text
115200
```

A successful startup should look similar to:

```text
================================
ESP32 Wake-on-LAN MQTT
================================

Connecting to Wi-Fi...

Wi-Fi connected!
ESP32 IP: 192.168.x.x

Connecting to HiveMQ... connected!
Subscribed to: home/pc/wake
[xxxx ms] MQTT subscription ready.

ESP32 ready.
```

# 9. Test the MQTT command

You can publish a `WAKE` message using any MQTT client.

On Windows, the command can be written on one line:

```cmd
"C:\Program Files\mosquitto\mosquitto_pub.exe" --cafile "C:\path\to\isrgrootx1.pem" -h "YOUR_MQTT_SERVER_ADDRESS" -p 8883 -u "YOUR_MQTT_USERNAME" -P "YOUR_MQTT_PASSWORD" -t "home/pc/wake" -m "WAKE" -q 1
```

You can also use:

- A phone MQTT client
- Home Assistant
- Node-RED
- A script
- Another computer
- Any application capable of publishing MQTT messages

The important part is:

```text
Topic:   home/pc/wake
Payload: WAKE
```

# 10. Tailscale Setup

Add Tailscale to the post-boot access workflow so your remote device can reach the PC after it starts. Install Tailscale on both the target PC and the machine that will run helper scripts (your laptop or server).

Download installers: [Tailscale downloads](https://tailscale.com/download)

Windows (MSI):

```powershell
# download and run the installer, then sign in
winget install --id=Tailscale.Tailscale
tailscale up
```

Linux (Debian/Ubuntu):

```bash
curl -fsSL https://tailscale.com/install.sh | sh
sudo tailscale up
```

After signing in, confirm connectivity:

```cmd
tailscale status
tailscale ping <PC-hostname-or-addr>
```

Notes:

- Use the PC's Tailscale hostname or Tailscale IP in scripts such as `wakepc.bat` and `playpc.bat` to check when the PC becomes reachable.
- If you have customized your Tailscale access policies, make sure they allow the remote device to access the required services such as SSH or RDP.
- Tailscale is only needed after the PC boots; it cannot wake a powered-off machine.

# 11. Moonlight and Sunshine

You can use Moonlight and Sunshine to stream the PC to another device.

- [Moonlight](https://moonlight-stream.org/) is the streaming client.
- [Sunshine](https://github.com/LizardByte/Sunshine) is the streaming host running on the gaming PC.

## On the target PC

Install Sunshine and configure its applications.

A common application is:

```text
Desktop
```

If the script contains:

```bat
set "APP=Desktop"
```

the script requests that application from Sunshine.

You can change it to another application configured in Sunshine.

For example:

```bat
set "APP=Steam Big Picture"
```

The name must match the application configured in Sunshine.

Sunshine should be configured to start automatically so that it is available after the PC boots.

## On the client machine

Install Moonlight and pair it with the target PC.

Pairing normally requires a PIN that Moonlight displays and Sunshine accepts.

After pairing has been completed, the helper script can launch Moonlight automatically.

# 12. Moonlight settings

The exact settings depend on the client display and network.

A typical command might look like:

```bat
start "" "%MOONLIGHT%" stream %PC% "%APP%" --1080 --fps 60 --bitrate 10000 --display-mode fullscreen --keep-awake
```

The main settings are:

| Option                      | Purpose                                                 |
| --------------------------- | ------------------------------------------------------- |
| `--1080`                    | Stream at 1080p                                         |
| `--fps 60`                  | Stream at 60 FPS                                        |
| `--bitrate 10000`           | 10 Mbps streaming bitrate                               |
| `--display-mode fullscreen` | Open the client in fullscreen                           |
| `--keep-awake`              | Prevent the client device from sleeping while streaming |

The example uses 1080p for compatibility. Adjust the resolution and bitrate to match your client display and available network bandwidth.

Higher bitrate generally improves image quality, particularly in gradients and other areas where compression artifacts are visible, but it also requires more network bandwidth.

# 13. Windows helper scripts

This project can optionally include Windows batch scripts to make the system easier to use from Command Prompt.

The Windows scripts can use:

- [Mosquitto](https://mosquitto.org/download/) for publishing MQTT commands
- [Tailscale](https://tailscale.com/) for Tailscale for private connectivity across networks/NAT
- [Moonlight](https://moonlight-stream.org/) for game/desktop streaming
- [Sunshine](https://github.com/LizardByte/Sunshine) as the streaming host

## `wakepc.bat`

The wake script:

1. Publishes `WAKE` to the MQTT topic.
2. Waits for the target PC to become reachable.
3. Reports when the PC is online.

## `playpc.bat`

The play script:

1. Publishes `WAKE`.
2. Waits for the target PC to become reachable.
3. Waits additional time for Windows and Sunshine to become ready.
4. Launches Moonlight.
5. Starts the configured Sunshine application.

## `statuspc.bat`

The status script checks whether the target PC is currently reachable and reports its status.

Tip: place these helper scripts in `%USERPROFILE%` (or any folder on your `PATH`) so you can run them directly from Command Prompt without specifying a path.

# 14. Troubleshooting

| Symptom                                            | Likely cause                                                                                                                         |
| -------------------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------ |
| Wi-Fi never connects                               | Incorrect SSID/password, weak signal, or the ESP32 is attempting to use a 5 GHz network                                              |
| `MQTT FAILED, MQTT state = -2`                     | Broker address/port problem or TLS connection failure                                                                                |
| `MQTT FAILED, MQTT state = 4`                      | MQTT credentials are incorrect                                                                                                       |
| `MQTT FAILED, MQTT state = 5`                      | MQTT credentials are not authorized                                                                                                  |
| ESP32 connects and subscribes but PC does not wake | WOL is disabled, incorrect`PC_MAC`, incorrect`BROADCAST_IP`, unsupported adapter, or network configuration issue                     |
| WOL works from sleep but not shutdown              | Check BIOS/UEFI settings, Windows Fast Startup, and network adapter power settings                                                   |
| Broker certificate verification fails              | The configured CA certificate does not match the broker's certificate chain                                                          |
| `wakepc`is not recognized                          | The directory containing the batch file is not on the Windows`PATH`                                                                  |
| `mosquitto_pub`cannot connect                      | Check broker address, credentials, port, CA certificate, and MQTT configuration                                                      |
| PC appears offline after waking                    | Windows or Tailscale may still be starting                                                                                           |
| `playpc`launches too early                         | Increase the delay after the PC becomes reachable                                                                                    |
| Moonlight cannot connect                           | Check Sunshine, pairing, firewall rules, hostname/address, and network connectivity                                                  |
| Moonlight stream has visible color banding         | Increase streaming bitrate, use an appropriate codec, and make sure the source and client are configured for the desired color depth |
| Tailscale cannot connect directly                  | The devices may be behind restrictive NAT/firewalls; Tailscale may fall back to a DERP relay                                         |

# 15. Why use MQTT instead of exposing WOL directly?

Traditional remote WOL setups often require a router to expose a service or forward UDP traffic into the home network.

That can become complicated when:

- The ISP uses CGNAT
- The router does not support directed broadcast
- The public IPv4 address changes
- You don't want to expose a UDP port
- You want centralized message logging/monitoring
- You want to trigger the wake command from different devices

With this project, the ESP32 maintains the MQTT connection from inside the home network.

The remote device only needs to publish an MQTT message:

```text
home/pc/wake → WAKE
```

The ESP32 then performs the local WOL operation.

This avoids requiring the WOL packet itself to cross the internet.

## License

This project is licensed under the MIT License. See [LICENSE](LICENSE) for details.
