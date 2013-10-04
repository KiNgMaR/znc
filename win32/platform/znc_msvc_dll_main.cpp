#ifdef HAVE_LIBSSL

#include "openssl/applink.c"
// for linking with OpenSSL DLLs
// see http://www.openssl.org/support/faq.html
// Failure to link this shim module into your application manifests itself as fatal
// "no OPENSSL_Applink" run-time error.

#endif /* HAVE_LIBSSL */

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	return TRUE;
}
