# Win32 APIについて

## ANSIとUnicode

Win32の各APIにはANSI版とUnicode版が用意されていて、ANSI版にはA、Unicode版にはWが関数のどこかしらについている。<br/>
どちらもつかないのは抽象化の為のマクロで、何もしなければANSI版に展開され、

```c
#define UNICODE
```

されているとUnicode版に展開される。

作法としては

```c
#ifndef UNICODE
#define UNICODE
#endif
```

とすることが望ましい。
