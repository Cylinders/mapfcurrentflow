#pragma once

#include "problem/debug.h"
#include "problem/map.h"
#include "types/basic_types.h"
#include "types/file_system.h"
#include "types/matrix.h"
#include "types/string.h"
#include "types/vector.h"

struct AgentData
{
    Node start;
    Node goal;

    bool operator==(const AgentData&) const = default;
};

struct BcpInstance
{
    // Instance
    String name;
    FilePath scenario_path;
    FilePath map_path;

    // Map
    Map map;

    // Agents
    Agent num_agents;
    Vector<AgentData> agents;
    Time time_horizon;

  public:
    // Constructors and destructor
    BcpInstance(const FilePath& scenario_path, const Agent agent_limit);
    BcpInstance() = delete;
    BcpInstance(const BcpInstance&) = delete;
    BcpInstance(BcpInstance&&) = delete;
    BcpInstance& operator=(const BcpInstance&) = delete;
    BcpInstance& operator=(BcpInstance&&) = delete;
    ~BcpInstance() = default;

    // added for mapf-dallas. not original.
    BcpInstance(const String &name, const Map map, const Vector<AgentData> agents);
};
