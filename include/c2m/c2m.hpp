#pragma once

namespace c2m {
	int ceilToMultipleOf16(int x) {
		return (x + 15) & ~15;
	}
}