# HTTP Client library and protocol for UDJAT.

[![License: GPL v3](https://img.shields.io/badge/License-GPL%20v3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)
![CodeQL](https://github.com/PerryWerneck/udjat-module-http/workflows/CodeQL/badge.svg?branch=master)
[![build result](https://build.opensuse.org/projects/home:PerryWerneck:udjat/packages/udjat-module-http/badge.svg?type=percent)](https://build.opensuse.org/package/show/home:PerryWerneck:udjat/udjat-module-http)

# Sample use

## Authenticated get

```C++
	cout    <<
			HTTP::Client("http://localhost/sample")
				.setCredentials("userid","passwd")
				.get()
			<< endl;
```

