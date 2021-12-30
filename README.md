# udjat-module-protocol-http
HTTP Client library and protocol for UDJAT.

# Sample use

## Authenticated get

```C++
	cout    <<
			HTTP::Client("http://localhost/sample")
				.setCredentials("userid","passwd")
				.get()
			<< endl;
```

