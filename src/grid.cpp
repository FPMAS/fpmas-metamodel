#include "grid.h"

float UniformUtility::utility(Attractor, DiscretePoint) const {
	return 1.f;
}

float LinearUtility::utility(Attractor attractor, DiscretePoint point) const {
	return std::max(
			0.f, attractor.radius - fpmas::api::model::euclidian_distance(
				attractor.center, point
				)
			);
}

float InverseUtility::utility(Attractor attractor, DiscretePoint point) const {
	// 1/x like utility function depending on the distance from the center.
	// Utility=1 at center
	// Utility=beta when distance=radius
	float beta = 0.5;
	float alpha = (1 - beta) / (beta * attractor.radius);
	return 1 / (1 + alpha * (fpmas::api::model::euclidian_distance(
				attractor.center, point
				)-offset));
}

float StepUtility::utility(Attractor attractor, DiscretePoint point) const {
	if(fpmas::api::model::euclidian_distance(
			attractor.center, point
			) > attractor.radius)
		return InverseUtility(attractor.radius).utility(attractor, point);
	else
		return 1.f;
}

BenchmarkCell* BenchmarkCellFactory::build(fpmas::model::DiscretePoint location) {
	float utility = 0;
	for(auto attractor : attractors) {
		utility += utility_function.utility(attractor, location);
	}
	return new BenchmarkCell(location, utility);
}

