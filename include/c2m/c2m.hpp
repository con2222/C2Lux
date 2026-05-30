#pragma once

namespace c2m {
	int ceilToMultipleOf16(int x) {
		return static_cast<int>(16 * std::ceil(x / 16.0));
	}
}