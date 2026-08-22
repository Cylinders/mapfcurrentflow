#pragma once

#include <filesystem>
#include <span>

#include "onnxruntime_cxx_api.h"
#include "valuator.h"

namespace mapf::meta {
    struct MapfasterValuator final : Valuator<MapfasterPortfolio> {
    public:
        explicit MapfasterValuator(const std::filesystem::path &model_path);

        [[nodiscard]]
        std::vector<std::array<SolverWeight, 4> > evaluate_batch(const Grid &grid,
                                                                 std::span<const std::vector<Agent>> agents_sets,
                                                                 const std::unordered_map<Agent, std::vector<Pos> > &paths) override;

    private:
        static constexpr std::array input_names{"image"};
        static constexpr std::array output_names{"prediction"};

        Ort::Env env_;
        Ort::Session session_;
        Ort::MemoryInfo memory_info_;
    };
}
