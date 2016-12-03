#include <stdio.h>
#include <stdlib.h>
#include <libuhid.h>
int main()
{
    hid_device *d = uhidOpen(NULL);
    printf("%s \n", uhidmgrDevicePath(d, "blah"));

}
