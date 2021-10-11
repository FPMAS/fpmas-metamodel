#include "config.h"

#define LOAD_YAML_CONFIG(FIELD_NAME, TYPENAME)\
	{\
	YAML::Node node = config[#FIELD_NAME];\
	if(!node.IsDefined()) {\
		std::cerr << "[FATAL ERROR] Missing configuration field: " #FIELD_NAME " (" #TYPENAME ")" << std::endl;\
		this->is_valid = false;\
	} else {\
		try{\
			this->FIELD_NAME = node.as<TYPENAME>();\
		} catch (const YAML::TypedBadConversion<TYPENAME>& e) {\
			this->is_valid = false;\
			std::cerr << "[FATAL ERROR] Bad " #FIELD_NAME " field parsing. Expected type: " #TYPENAME << std::endl;\
		}\
	}\
	}

BenchmarkConfig::BenchmarkConfig(std::string config_file) {
	try {
		YAML::Node config = YAML::LoadFile(config_file);

		LOAD_YAML_CONFIG(grid_width, unsigned int);
		LOAD_YAML_CONFIG(grid_height, unsigned int);
		LOAD_YAML_CONFIG(occupation_rate, float);
		LOAD_YAML_CONFIG(num_steps, fpmas::api::scheduler::TimeStep);
		LOAD_YAML_CONFIG(utility, Utility);
		LOAD_YAML_CONFIG(attractors, std::vector<Attractor>);
		LOAD_YAML_CONFIG(agent_interactions, AgentInteractions);
		LOAD_YAML_CONFIG(test_cases, std::vector<TestCaseConfig>);
	} catch(const YAML::BadFile&) {
		this->is_valid = false;
		std::cerr << "[FATAL ERROR] Config file not found: " << config_file << std::endl;
	}
}

namespace YAML {
	Node convert<Utility>::encode(const Utility& utility) {
		switch(utility) {
			case UNIFORM:
				return Node("UNIFORM");
			case LINEAR:
				return Node("LINEAR");
			case INVERSE:
				return Node("INVERSE");
			case STEP:
				return Node("STEP");
			default:
				return Node();
		}
	}

	bool convert<Utility>::decode(const Node &node, Utility& utility) {
		std::string str = node.as<std::string>();
		if(str == "UNIFORM") {
			utility = UNIFORM;
			return true;
		}
		if(str == "LINEAR") {
			utility = LINEAR;
			return true;
		}
		if(str == "INVERSE") {
			utility = INVERSE;
			return true;
		}
		if(str == "STEP") {
			utility = STEP;
			return true;
		}
		return false;
	}

	Node convert<LbAlgorithm>::encode(const LbAlgorithm& lb_algorithm) {
		switch(lb_algorithm) {
			case SCHEDULED_LB:
				return Node("SCHEDULED_LB");
			case ZOLTAN_LB:
				return Node("ZOLTAN_LB");
			case GRID_LB:
				return Node("GRID_LB");
			case ZOLTAN_CELL_LB:
				return Node("ZOLTAN_CELL_LB");
			case RANDOM_LB:
				return Node("RANDOM_LB");
			default:
				return Node();
		}
	}

	bool convert<LbAlgorithm>::decode(const Node &node, LbAlgorithm& lb_algorithm) {
		std::string str = node.as<std::string>();
		if(str == "SCHEDULED_LB") {
			lb_algorithm = SCHEDULED_LB;
			return true;
		}
		if(str == "ZOLTAN_LB") {
			lb_algorithm = ZOLTAN_LB;
			return true;
		}
		if(str == "GRID_LB") {
			lb_algorithm = GRID_LB;
			return true;
		}
		if(str == "ZOLTAN_CELL_LB") {
			lb_algorithm = ZOLTAN_CELL_LB;
			return true;
		}
		if(str == "RANDOM_LB") {
			lb_algorithm = RANDOM_LB;
			return true;
		}
		return false;
	}

	Node convert<AgentInteractions>::encode(
			const AgentInteractions& agent_interactions) {
		switch(agent_interactions) {
			case LOCAL:
				return Node("LOCAL");
			case SMALL_WORLD:
				return Node("SMALL_WORLD");
			default:
				return Node();
		}
	}

	bool convert<AgentInteractions>::decode(
			const Node &node,
			AgentInteractions &agent_interactions) {
		std::string str = node.as<std::string>();
		if(str == "LOCAL") {
			agent_interactions = LOCAL;
			return true;
		}
		if(str == "SMALL_WORLD") {
			agent_interactions = SMALL_WORLD;
			return true;
		}
		return false;
	}

	Node convert<Attractor>::encode(const Attractor& attractor) {
		Node point;
		point.push_back(attractor.center.x);
		point.push_back(attractor.center.y);

		Node node;
		node.push_back(point);
		node.push_back(attractor.radius);
		return node;
	}

	bool convert<Attractor>::decode(const Node &node, Attractor& attractor) {
		// The root node contains 2 elements
		if(!node.IsSequence() || node.size() != 2)
			return false;
		// The first is a point
		if(!node[0].IsSequence() || node.size() != 2)
			return false;
		// The second is the radius
		if(!node[1].IsScalar())
			return false;

		attractor.center = {
			node[0][0].as<fpmas::api::model::DiscreteCoordinate>(),
			node[0][1].as<fpmas::api::model::DiscreteCoordinate>(),
		};
		attractor.radius = node[1].as<float>();

		return true;
	}

	Node convert<TestCaseConfig>::encode(const TestCaseConfig& test_case_config) {
		Node node;
		node.push_back(test_case_config.algorithm);
		node.push_back(test_case_config.lb_periods);
		return node;
	}

	bool convert<TestCaseConfig>::decode(const Node &node, TestCaseConfig& test_case_config) {
		if(!node.IsSequence() || node.size() != 2)
			return false;
		test_case_config.algorithm = node[0].as<LbAlgorithm>();
		test_case_config.lb_periods = node[1].as<std::vector<fpmas::api::scheduler::TimeStep>>();
		return true;
	}
}
