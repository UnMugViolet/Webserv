# 💾 Webserv 

### A 42 school projet aiming to build a webserver from scratch using only C++.


<p align="center">
	<img src="img/webserv.png" alt="webserv Badge" width="15%">
</p>


## Introduction

Add some explaination here

## 💿 Install

```sh
sudo apt update && sudo apt upgrade -y
sudo apt install siege -y
sudo apt install g++ -y
sudo apt intall curl -y
make
```

## 🐩 Usage

### Run the server with the default configuration file:
```sh
make go
```

### Run the stress test with siege:
```sh
make stress
```

### Run the request test with curl:
```sh 
make curl
```

## Structure

- Webserv (main server class)
- ConfigParser (configuration parser)
- Server (individual server instance)
- Request (HTTP request parser)
- Response (HTTP response builder)
- CGI (CGI handler)
- Client (client connection handler)

## 🤼 Credits

Made with Antonin Drean | Yann Guinio | Paul Jaguin. 
The official Leak of Legends team.


## Doc used

- [webserv: Building a Non-Blocking Web Server in C++98 (A 42 project)](https://m4nnb3ll.medium.com/webserv-building-a-non-blocking-web-server-in-c-98-a-42-project-04c7365e4ec7)
- [codequoi: Documentation sur les principes techniques derriere webserv](https://www.codequoi.com/programmation-reseau-via-socket-en-c/)

## 📜 License

This project is open-sourced software licensed under the [MIT license](https://opensource.org/licenses/MIT).
