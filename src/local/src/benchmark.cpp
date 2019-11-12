#include <stdlib.h>

#include "pmc_utils.h"

int main(int argc, char *argv[])
{
	pmc_begin();
	pmc_setup();
	pmc_end();

	exit(0);
}
