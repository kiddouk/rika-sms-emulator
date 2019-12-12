#include "WProgram.h"

extern "C" int main(void)
{
	// Arduino's main() function just calls setup() and loop()....
	setup();
	while (1) {
		loop();
		yield();
	}
}

