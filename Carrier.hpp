#pragma once

class Carrier {
public:
	int w, h, d;
	//Volume volume;
	int volume;
	float threshold;


	int intensity(glm::vec3 p);
};