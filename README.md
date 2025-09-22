# FT_IRC

## 👥 Team
This project was developed by:
![mfortuna]((https://github.com/mfortuna23))
![alfreire]((https://github.com/Alex-mfs))

## 📖 Overview

**ft_irc** is a custom IRC (Internet Relay Chat) server implementation written in C++98, developed as a group project at 42 School. This server complies with the RFC 1459 and RFC 2812 standards and supports multiple clients connecting simultaneously through a single process using poll() for I/O multiplexing.

## 🚀 Features

- **RFC-Compliant**: Implements major IRC standards
- **Multi-Client Support**: Handles multiple clients simultaneously
- **Channel Operations**: 
  - Create/join channels (`#channel`)
  - Channel modes (`+i`, `+t`, `+k`, `+o`, `+l`)
  - Topic management
- **Private Messaging**: Direct messages between users
- **Operator Privileges**: Channel operators can kick/ban users and manage modes
- **Network Communication**: Full server-client communication protocol

## 📋 Supported Commands

| Command | Description | Example |
|---------|-------------|---------|
| `PASS` | Set connection password | `PASS 12345` |
| `NICK` | Set nickname | `NICK john` |
| `USER` | Set username/realname | `USER jdoe 0 * :John Doe` |
| `JOIN` | Join/create channel | `JOIN #general` |
| `PART` | Leave channel | `PART #general` |
| `PRIVMSG` | Send message | `PRIVMSG #general :Hello!` |
| `TOPIC` | Set channel topic | `TOPIC #general :Welcome!` |
| `MODE` | Set channel modes | `MODE #general +i` |
| `KICK` | Remove user from channel | `KICK #general john` |
| `INVITE` | Invite user to channel | `INVITE jane #general` |
| `QUIT` | Disconnect from server | `QUIT :Goodbye!` |

## 🛠️ Installation & Usage

### Prerequisites
- C++98 compatible compiler
- Make
- IRC client (HexChat, irssi, etc.) for testing

### Building
```bash
git clone <your-repo-url>
cd ft_irc
```
### Make
Running the Server
```bash
./ircserver <port> <password>
# Example:
./ircserver 6667 12345
```
Connecting Clients
```bash
# Using HexChat:
# Server: localhost:6667, Password: 12345

# Using netcat:
nc localhost 6667
PASS 12345
NICK yournick
USER youruser 0 * :Your Name
```
##🏗️ Project Structure
```text
ft_irc/
├── src/                 # Source files
│   ├── Server.cpp       # Main server logic
│   ├── Client.cpp       # Client management
│   ├── Channel.cpp      # Channel system
│   └── Commands.cpp     # Command handlers
├── includes/            # Header files
├── Makefile            # Build configuration
└── README.md           # This file
```

## 🔧 Technical Details
I/O Model: Single-threaded with poll() for multiplexing

Protocol: RFC 1459/2812 compliant IRC protocol

Memory: Manual memory management with proper cleanup

Error Handling: Comprehensive error codes and recovery

# 📚 Resources

![RFC 1459: IRC Protocol](https://datatracker.ietf.org/doc/html/rfc1459)

![RFC 2812: Modern IRC](https://datatracker.ietf.org/doc/html/rfc2812)

![IRC Docs Horse](https://modern.ircdocs.horse)

# ⚠️ Notes
Developed following 42 School coding standards

Requires C++98 compatibility

Includes comprehensive error handling and memory management

#📄 License
This project is part of the 42 School curriculum. All rights reserved.

