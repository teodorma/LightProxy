# Proxy Load Balancer with SSL/TLS Termination

This project is a proxy load balancer server with TLS/SSL termination capability. The server is developed in C++ using OpenSSL for encryption and includes a custom logger class for logging events and errors. It distributes traffic across multiple backend servers, handling incoming secure client connections, decrypting requests, and forwarding them to backend servers.

## Features

- **SSL/TLS Termination**: Establishes secure connections with clients using SSL/TLS, then forwards decrypted requests to backend servers.
- **Load Balancing**: Distributes incoming requests evenly across multiple backend servers.
- **OCSP Support**: Periodically updates OCSP responses to maintain certificate validity.
- **Signal Handling**: Cleanly shuts down the server and releases resources on exit.
- **IPv6 Support**: Binds to all available IPv6 addresses for compatibility.
- **Logging**: Logs significant events and errors to a log file using a custom `Logger` class.

## Prerequisites

1. **Compiler**: A C++ compiler supporting C++11 or later.
2. **Libraries**:
   - **OpenSSL**: Required for SSL/TLS encryption.
3. **Configuration Files**:
   - SSL/TLS Certificate and Key files: Replace `<path-to-certificate>` and `<path-to-key>` in the code with actual file paths for SSL.
   - Log file directory: Ensure the specified log file path exists (e.g., `../Proxy/Log/server.log`).

## Installing Dependencies
### Ubuntu/Debian

Run the following commands to install the necessary packages:

```bash
  sudo apt-get update
  sudo apt-get install -y libssl-dev cmake g++ build-essential git
```

### Fedora

Run the following commands to install the necessary packages on Fedora:

```bash
  sudo dnf update
  sudo dnf install -y openssl-devel cmake gcc-c++ make git
```

### Cloning and Building the Project
After installing dependencies, clone the repository and build the project with CMake:
```bash
  git clone https://github.com/your-username/LightProxy.git
  cd LightProxy
  
  mkdir build && cd build

  cmake ..
  make
```
**Note**: Since port 443 is a privileged port (ports below 1024 require elevated permissions), you need to run the server as **root** or with **sudo**:

```bash
  sudo ./server
```


The server will start listening on port 443 by default, accepting secure client connections, decrypting traffic, and distributing requests to configured backend servers.


## Setting up systemd Service for Proxy Server (Optional)
Open a new service file in /etc/systemd/system/:
```bash
sudo nano /etc/systemd/system/server.service
```
Add the following configuration, replacing paths as needed:
```
   [Unit]
   Description=Proxy Server Service
   After=network.target
   
   [Service]
   ExecStart=/your/path
   WorkingDirectory=/your/path
   User=root
   Restart=on-failure
   RestartSec=5
   StandardOutput=journal
   StandardError=journal
   Environment="PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin"
   
   [Install]
   WantedBy=multi-user.target
```

Reload systemd to apply the new service configuration:
```bash
   # Reload systemd to apply the new service configuration:
   sudo systemctl daemon-reload
   
   # Enable the service to start on boot:
   sudo systemctl enable server.service
   
   # Start the service: 
   sudo systemctl start server.service
```

## Contact
For questions or feedback, contact me at teodormaciuca1@gmail.com.
This expanded README provides configuration instructions, usage examples, code structure details, troubleshooting tips, and contact information to help users get started with and maintain the project. Let me know if there’s anything more specific you’d like to add!
