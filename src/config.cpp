#include "agent.h"
#include "config.h"

#define LOAD_YAML_CONFIG_0(FIELD_NAME, TYPENAME)\
	load_config(#FIELD_NAME, FIELD_NAME, config[#FIELD_NAME], #TYPENAME)
#define LOAD_YAML_CONFIG_0_OPTIONAL(FIELD_NAME, TYPENAME, DEFAULT)\
	load_config_optional(#FIELD_NAME, FIELD_NAME, config[#FIELD_NAME], #TYPENAME, DEFAULT)
#define LOAD_YAML_CONFIG_1(ROOT, FIELD_NAME, TYPENAME)\
	load_config(#ROOT "::" #FIELD_NAME, ROOT::FIELD_NAME, config[#ROOT][#FIELD_NAME], #TYPENAME)
#define LOAD_YAML_CONFIG_1_OPTIONAL(ROOT, FIELD_NAME, TYPENAME, DEFAULT)\
	load_config_optional(#ROOT "::" #FIELD_NAME, ROOT::FIELD_NAME, config[#ROOT][#FIELD_NAME], #TYPENAME, DEFAULT)


GraphConfig::GraphConfig(YAML::Node config) {
	LOAD_YAML_CONFIG_0(environment, Environment);
	switch(this->environment) {
		case Environment::GRID:
			LOAD_YAML_CONFIG_0(grid_width, unsigned int);
			LOAD_YAML_CONFIG_0(grid_height, unsigned int);
			break;
		case Environment::SMALL_WORLD:
			LOAD_YAML_CONFIG_0(p, float);
		case Environment::CLUSTERED:
		case Environment::RANDOM:
			LOAD_YAML_CONFIG_0(num_cells, unsigned int);
			LOAD_YAML_CONFIG_0(output_degree, unsigned int);
	}
	LOAD_YAML_CONFIG_0_OPTIONAL(cell_weight, float, 1.0f);
	LOAD_YAML_CONFIG_1_OPTIONAL(MetaCell, cell_edge_weight, float, 1.0f);
	LOAD_YAML_CONFIG_0_OPTIONAL(utility, Utility, Utility::UNIFORM);
	if(this->utility != Utility::UNIFORM)
		switch(this->environment) {
			case Environment::GRID:
				LOAD_YAML_CONFIG_0(grid_attractors, std::vector<GridAttractor>);
				break;
			default:
				break;
				// TODO: Graph based attractors
		}
	LOAD_YAML_CONFIG_0_OPTIONAL(zoltan_imbalance_tol, float, 1.1f);
	LOAD_YAML_CONFIG_0_OPTIONAL(json_output, bool, false);
	LOAD_YAML_CONFIG_0_OPTIONAL(json_output_period, int, -1);
	LOAD_YAML_CONFIG_0_OPTIONAL(dot_output, bool, false);
}

ModelConfig::ModelConfig(const GraphConfig& graph_config)
	: GraphConfig(graph_config) {
	}

ModelConfig::ModelConfig(YAML::Node config) : GraphConfig(config) {
	LOAD_YAML_CONFIG_0(occupation_rate, float);
	LOAD_YAML_CONFIG_0(num_steps, fpmas::api::scheduler::TimeStep);
	if(this->occupation_rate > 0.0) {
		LOAD_YAML_CONFIG_0_OPTIONAL(agent_weight, float, 1.0f);
		LOAD_YAML_CONFIG_0_OPTIONAL(
				agent_interactions, AgentInteractions, AgentInteractions::LOCAL
				);
		if(this->agent_interactions == AgentInteractions::CONTACTS) {
			LOAD_YAML_CONFIG_0(refresh_local_contacts, fpmas::api::scheduler::TimeStep);
			LOAD_YAML_CONFIG_0(refresh_distant_contacts, fpmas::api::scheduler::TimeStep);
			LOAD_YAML_CONFIG_1_OPTIONAL(MetaAgentBase, contact_weight, float, 1.0f);
			LOAD_YAML_CONFIG_1(MetaAgentBase, max_contacts, unsigned int);
		}
	}
	LOAD_YAML_CONFIG_0_OPTIONAL(cell_interactions, Interactions, Interactions::NONE);
	LOAD_YAML_CONFIG_0_OPTIONAL(dynamic_cell_edge_weights, bool, false);
	LOAD_YAML_CONFIG_0_OPTIONAL(sync_mode, SyncMode, SyncMode::GHOST_MODE);
	LOAD_YAML_CONFIG_0_OPTIONAL(cell_size, std::size_t, (std::size_t) 0);
	LOAD_YAML_CONFIG_1_OPTIONAL(
			MetaAgentBase, move_policy, MovePolicy, MovePolicy::RANDOM);
	LOAD_YAML_CONFIG_1_OPTIONAL(
			MetaAgentBase, range_size, unsigned int, (std::size_t) 1);
	LOAD_YAML_CONFIG_0(test_cases, std::vector<TestCaseConfig>);
}

namespace YAML {
	Node convert<Environment>::encode(const Environment& graph_type) {
		switch(graph_type) {
			case Environment::GRID:
				return Node("GRID");
			case Environment::RANDOM:
				return Node("RANDOM");
			case Environment::CLUSTERED:
				return Node("CLUSTERED");
			case Environment::SMALL_WORLD:
				return Node("SMALL_WORLD");
			default:
				return Node();
		}
	}

	bool convert<Environment>::decode(const Node &node, Environment& graph_type) {
		std::string str = node.as<std::string>();
		if(str == "GRID") {
			graph_type = Environment::GRID;
			return true;
		}
		if(str == "RANDOM") {
			graph_type = Environment::RANDOM;
			return true;
		}
		if(str == "CLUSTERED") {
			graph_type = Environment::CLUSTERED;
			return true;
		}
		if(str == "SMALL_WORLD") {
			graph_type = Environment::SMALL_WORLD;
			return true;
		}
		return false;
	}

	Node convert<Utility>::encode(const Utility& utility) {
		switch(utility) {
			case Utility::UNIFORM:
				return Node("UNIFORM");
			case Utility::LINEAR:
				return Node("LINEAR");
			case Utility::INVERSE:
				return Node("INVERSE");
			case Utility::STEP:
				return Node("STEP");
			default:
				return Node();
		}
	}

	bool convert<Utility>::decode(const Node &node, Utility& utility) {
		std::string str = node.as<std::string>();
		if(str == "UNIFORM") {
			utility = Utility::UNIFORM;
			return true;
		}
		if(str == "LINEAR") {
			utility = Utility::LINEAR;
			return true;
		}
		if(str == "INVERSE") {
			utility = Utility::INVERSE;
			return true;
		}
		if(str == "STEP") {
			utility = Utility::STEP;
			return true;
		}
		return false;
	}

	Node convert<MovePolicy>::encode(const MovePolicy& move_policy) {
		switch(move_policy) {
			case MovePolicy::RANDOM:
				return Node("RANDOM");
			case MovePolicy::MAX:
				return Node("MAX");
			default:
				return Node();
		}
	}

	bool convert<MovePolicy>::decode(const Node &node, MovePolicy& move_policy) {
		std::string str = node.as<std::string>();
		if(str == "RANDOM") {
			move_policy = MovePolicy::RANDOM;
			return true;
		}
		if(str == "MAX") {
			move_policy = MovePolicy::MAX;
			return true;
		}
		return false;
	}

	Node convert<LbAlgorithm>::encode(const LbAlgorithm& lb_algorithm) {
		switch(lb_algorithm) {
			case LbAlgorithm::SCHEDULED_LB:
				return Node("SCHEDULED_LB");
			case LbAlgorithm::ZOLTAN_LB:
				return Node("ZOLTAN_LB");
			case LbAlgorithm::GRID_LB:
				return Node("GRID_LB");
			case LbAlgorithm::ZOLTAN_CELL_LB:
				return Node("ZOLTAN_CELL_LB");
			case LbAlgorithm::STATIC_ZOLTAN_CELL_LB:
				return Node("STATIC_ZOLTAN_CELL_LB");
			case LbAlgorithm::RANDOM_LB:
				return Node("RANDOM_LB");
			default:
				return Node();
		}
	}

	bool convert<LbAlgorithm>::decode(const Node &node, LbAlgorithm& lb_algorithm) {
		std::string str = node.as<std::string>();
		if(str == "SCHEDULED_LB") {
			lb_algorithm = LbAlgorithm::SCHEDULED_LB;
			return true;
		}
		if(str == "ZOLTAN_LB") {
			lb_algorithm = LbAlgorithm::ZOLTAN_LB;
			return true;
		}
		if(str == "GRID_LB") {
			lb_algorithm = LbAlgorithm::GRID_LB;
			return true;
		}
		if(str == "ZOLTAN_CELL_LB") {
			lb_algorithm = LbAlgorithm::ZOLTAN_CELL_LB;
			return true;
		}
		if(str == "STATIC_ZOLTAN_CELL_LB") {
			lb_algorithm = LbAlgorithm::STATIC_ZOLTAN_CELL_LB;
			return true;
		}
		if(str == "RANDOM_LB") {
			lb_algorithm = LbAlgorithm::RANDOM_LB;
			return true;
		}
		return false;
	}

	Node convert<AgentInteractions>::encode(
			const AgentInteractions& agent_interactions) {
		switch(agent_interactions) {
			case AgentInteractions::LOCAL:
				return Node("LOCAL");
			case AgentInteractions::CONTACTS:
				return Node("CONTACTS");
			default:
				return Node();
		}
	}

	bool convert<AgentInteractions>::decode(
			const Node &node,
			AgentInteractions &agent_interactions) {
		std::string str = node.as<std::string>();
		if(str == "LOCAL") {
			agent_interactions = AgentInteractions::LOCAL;
			return true;
		}
		if(str == "CONTACTS") {
			agent_interactions = AgentInteractions::CONTACTS;
			return true;
		}
		return false;
	}

	Node convert<Interactions>::encode(
			const Interactions& interactions) {
		switch(interactions) {
			case Interactions::NONE:
				return Node("NONE");
			case Interactions::READ_ALL:
				return Node("READ_ALL");
			case Interactions::READ_ONE:
				return Node("READ_ONE");
			case Interactions::READ_ALL_WRITE_ONE:
				return Node("READ_ALL_WRITE_ONE");
			case Interactions::READ_ALL_WRITE_ALL:
				return Node("READ_ALL_WRITE_ONE");
			case Interactions::WRITE_ALL:
				return Node("WRITE_ALL");
			case Interactions::WRITE_ONE:
				return Node("WRITE_ONE");
			default:
				return Node();
		}
	}

	bool convert<Interactions>::decode(
			const Node &node,
			Interactions &interactions) {
		std::string str = node.as<std::string>();
		if(str == "NONE") {
			interactions = Interactions::NONE;
			return true;
		}
		if(str == "READ_ALL") {
			interactions = Interactions::READ_ALL;
			return true;
		}
		if(str == "READ_ONE") {
			interactions = Interactions::READ_ONE;
			return true;
		}
		if(str == "READ_ALL_WRITE_ONE") {
			interactions = Interactions::READ_ALL_WRITE_ONE;
			return true;
		}
		if(str == "READ_ALL_WRITE_ALL") {
			interactions = Interactions::READ_ALL_WRITE_ALL;
			return true;
		}
		if(str == "WRITE_ALL") {
			interactions = Interactions::WRITE_ALL;
			return true;
		}
		if(str == "WRITE_ONE") {
			interactions = Interactions::WRITE_ONE;
			return true;
		}
		return false;
	}

	Node convert<SyncMode>::encode(
			const SyncMode& sync_mode) {
		switch(sync_mode) {
			case SyncMode::GHOST_MODE:
				return Node("GHOST_MODE");
			case SyncMode::GLOBAL_GHOST_MODE:
				return Node("GLOBAL_GHOST_MODE");
			case SyncMode::HARD_SYNC_MODE:
				return Node("HARD_SYNC_MODE");
			default:
				return Node();
		}
	}

	bool convert<SyncMode>::decode(
			const Node& node,
			SyncMode& sync_mode) {
		std::string str = node.as<std::string>();
		if(str == "GHOST_MODE") {
			sync_mode = SyncMode::GHOST_MODE;
			return true;
		}
		if(str == "GLOBAL_GHOST_MODE") {
			sync_mode = SyncMode::GLOBAL_GHOST_MODE;
			return true;
		}
		if(str == "HARD_SYNC_MODE") {
			sync_mode = SyncMode::HARD_SYNC_MODE;
			return true;
		}
		return false;
	}
		
	Node convert<Attractor>::encode(const Attractor& attractor) {
		Node node(attractor.radius);
		return node;
	}

	bool convert<Attractor>::decode(const Node &node, Attractor& attractor) {
		if(!node.IsScalar())
			return false;

		attractor.radius = node.as<float>();
		return true;
	}

	Node convert<GridAttractor>::encode(const GridAttractor& attractor) {
		Node point;
		point.push_back(attractor.center.x);
		point.push_back(attractor.center.y);

		Node node;
		node.push_back(point);
		node.push_back(attractor.radius);
		return node;
	}

	bool convert<GridAttractor>::decode(const Node &node, GridAttractor& attractor) {
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
