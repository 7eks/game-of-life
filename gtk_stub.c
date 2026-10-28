/* Stub per gtk_backend quando GTK non è compilato.
   Restituisce NULL, quindi main.c fallisce con errore chiaro. */
#include <stddef.h>
#include "render.h"

const RenderBackend *gtk_backend(void)
{
    return NULL;
}
