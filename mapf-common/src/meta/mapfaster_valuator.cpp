#include "mapf_common/meta/mapfaster_valuator.h"
#include "mapf_common/meta/mapfaster_encoder.h"
#include <iostream>

mapf::meta::MapfasterValuator::MapfasterValuator(const std::filesystem::path &model_path)
    : env_{ORT_LOGGING_LEVEL_WARNING, "mapfaster"},
    session_{nullptr},
      memory_info_{
          Ort::MemoryInfo::CreateCpu(
              OrtArenaAllocator,
              OrtMemTypeDefault
          )
      } {
    std::cout << "mapfaster: initializing...\n";
    const auto providers = Ort::GetAvailableProviders();

    for (const auto &provider: providers) {
        std::cout << "mapfaster: provider " << provider << std::endl;
    }

    if (std::ranges::find(providers, "CUDAExecutionProvider") == providers.end()) {
        throw std::runtime_error{"mapfaster: no gpu execution"};
    }

    Ort::SessionOptions options;
    options.SetGraphOptimizationLevel(
        ORT_ENABLE_ALL
    );

    OrtCUDAProviderOptions cuda_options{};
    cuda_options.device_id = 0;

    options.AppendExecutionProvider_CUDA(cuda_options);

    session_ = Ort::Session{
        env_,
        model_path.c_str(),
        options
    };
}

std::vector<std::array<mapf::meta::SolverWeight, 4> > mapf::meta::MapfasterValuator::evaluate_batch(
    const Grid &grid,
    const std::span<const std::vector<Agent>> agents_sets,
    const std::unordered_map<Agent, std::vector<Pos> > &paths
) {
    constexpr std::size_t solver_count = 4;

    const std::size_t batch_size = agents_sets.size();
    constexpr std::size_t encoded_size = mapfaster_encoded_input_size();

    std::vector<std::array<SolverWeight, solver_count> > results(batch_size);

    if (batch_size == 0) {
        return results;
    }

    std::vector<float> input_buffer(batch_size * encoded_size);

    for (std::size_t i = 0; i < batch_size; ++i) {
        mapfaster_encode_into(
            grid,
            agents_sets[i],
            paths,
            std::span{
                input_buffer.data() + i * encoded_size,
                encoded_size
            }
        );
    }

    std::array<int64_t, 4> input_shape{
        static_cast<int64_t>(batch_size),
        3,
        IMAGE_SIZE,
        IMAGE_SIZE
    };

    Ort::Value input_tensor = Ort::Value::CreateTensor<float>(
        memory_info_,
        input_buffer.data(),
        input_buffer.size(),
        input_shape.data(),
        input_shape.size()
    );

    auto outputs = session_.Run(
        Ort::RunOptions{nullptr},
        input_names.data(),
        &input_tensor,
        1,
        output_names.data(),
        1
    );

    const float *y = outputs[0].GetTensorData<float>();

    for (std::size_t batch_i = 0; batch_i < batch_size; ++batch_i) {
        for (std::size_t solver_i = 0; solver_i < solver_count; ++solver_i) {
            results[batch_i][solver_i] = SolverWeight{
                MapfasterPortfolio::entries[solver_i].kind,
                static_cast<double>(y[batch_i * solver_count + solver_i])
            };
        }
    }

    return results;
}
