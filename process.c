#include <stdlib.h>
#include "process.h"

static char* sh =	"tags=$(ps $1) \
					ok=no \
					for tag in tags \
					do \
					if [ \"$tag\" = \"root\" ] \
					then \
						ok=yes \
					fi \
					if [ \"$ok\" = \"yes\" ] \
					then \
						ok=no \
						kill $tag \
						exit \
					fi \
					done";

void pkill(const char* name)
{
	int pid;
	char cmd[1000] = "echo ";
	char tmp[1000];

	strcat(cmd, sh);
	strcat(cmd, " > /data/killsh");
	strcpy(cmd, "sh /data/killsh ");
	strcat(cmd, name);
	system(cmd);
	//system("rm /data/killsh");
}

