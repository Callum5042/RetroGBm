#include "Application.h"
#include <memory>

// Useful docs
// https://www.pastraiser.com/cpu/gameboy/gameboy_opcodes.html
// https://rgbds.gbdev.io/docs/v0.5.1/gbz80.7/#HALT
// https://archive.org/details/GameBoyProgManVer1.1/page/n1/mode/2up
// https://gekkio.fi/files/gb-docs/gbctr.pdf

int main(int argc, char** argv)
{
	std::unique_ptr<Application> application = std::make_unique<Application>();
	return application->Start();
}