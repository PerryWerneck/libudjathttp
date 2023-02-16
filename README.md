# HTTP Client library for UDJAT.

Windows/Linux http client library module for libudjat based applications.

[![License: GPL v3](https://img.shields.io/badge/License-GPL%20v3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)
![CodeQL](https://github.com/PerryWerneck/udjat-module-http/workflows/CodeQL/badge.svg?branch=master)
[![build result](https://build.opensuse.org/projects/home:PerryWerneck:udjat/packages/udjat-module-http/badge.svg?type=percent)](https://build.opensuse.org/package/show/home:PerryWerneck:udjat/udjat-module-http)

# Sample use

## As a library

### Authenticated get

```C++
cout    <<
	HTTP::Client("http://localhost/sample")
		.setCredentials("userid","passwd")
		.get()
<< endl;
```

### Post with variable parsing

This post expands ${} values with network info extracted from the active connection.

```C++
HTTP::Client("http://localhost").post(
	"ipaddr='${ipaddr}'\n"
	"hostip='${hostip}'\n"
	"nic='${network-interface}'\n"
	"macaddress='${macaddress}'"
);
```

### Save to file

```C++
HTTP::Client("http://localhost").save("test.html")
```
