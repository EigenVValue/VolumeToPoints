#include "thirdparties/include/glm/vec3.hpp"
#include "Carrier.hpp"
	/**
	 * An encapsulating class to avoid thread collisions on static fields.
	 */
class Carrier {
private:
	int w, h, d;
	//Volume volume;
	int volume;
	float threshold;

public:
	Carrier() {
	}

	int intensity(glm::vec3 p) {
		if (p.x < 0 || p.y < 0 || p.z < 0 || p.x >= w || p.y >= h || p.z >= d)
		{
			return 0;
		}
		//return volume.load((int)p.x, (int)p.y, (int)p.z);
		return volume;
	}

	float getThreshold() {
		return this->threshold;
	}
};