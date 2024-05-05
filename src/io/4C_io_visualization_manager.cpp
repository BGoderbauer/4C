/*-----------------------------------------------------------------------------------------------*/
/*! \file

\brief Define a container that holds the visualization data and the visualization writer

\level 0

*/
/*-----------------------------------------------------------------------------------------------*/

#include "4C_io_visualization_manager.hpp"

#include "4C_io_visualization_data.hpp"
#include "4C_io_visualization_parameters.hpp"
#include "4C_io_visualization_writer_factory.hpp"

#include <utility>

FOUR_C_NAMESPACE_OPEN

/**
 *
 */
IO::VisualizationManager::VisualizationManager(
    IO::VisualizationParameters parameters, const Epetra_Comm& comm, std::string base_output_name)
    : parameters_(std::move(parameters)),
      comm_(comm),
      base_output_name_(std::move(base_output_name))
{
}

/**
 *
 */
const IO::VisualizationData& IO::VisualizationManager::GetVisualizationData(
    const std::string& visualization_data_name) const
{
  if (visualization_map_.find(visualization_data_name) == visualization_map_.end())
  {
    FOUR_C_THROW("The requested visualization data \"%s\" is not registered.",
        visualization_data_name.c_str());
  }
  return visualization_map_.at(visualization_data_name).first;
}

/**
 *
 */
IO::VisualizationData& IO::VisualizationManager::GetVisualizationData(
    const std::string& visualization_data_name)
{
  if (visualization_map_.find(visualization_data_name) == visualization_map_.end() &&
      visualization_data_name == "")
  {
    // The default visualization data is registered the first time it is requested
    RegisterVisualizationData("");
  }
  // The const_cast can be done here, since we know that the underlying data is not const
  return const_cast<VisualizationData&>(
      const_cast<const VisualizationManager&>(*this).GetVisualizationData(visualization_data_name));
}

/**
 *
 */
IO::VisualizationData& IO::VisualizationManager::RegisterVisualizationData(
    const std::string& visualization_data_name)
{
  if (visualization_map_.find(visualization_data_name) != visualization_map_.end())
  {
    FOUR_C_THROW(
        "You are trying to register visualization data with the name \"%s\" but the "
        "visualization data is already registered, this is not possible",
        visualization_data_name.c_str());
  }
  visualization_map_[visualization_data_name] = std::make_pair(
      VisualizationData(), VisualizationWriterFactory(parameters_, comm_,
                               GetVisualizationDataNameForOutputFiles(visualization_data_name)));
  return visualization_map_[visualization_data_name].first;
}

/**
 *
 */
bool IO::VisualizationManager::VisualizationDataExists(
    const std::string& visualization_data_name) const
{
  return visualization_map_.find(visualization_data_name) != visualization_map_.end();
}

/**
 *
 */
void IO::VisualizationManager::ClearData()
{
  for (auto& [key, visualization_pair] : visualization_map_) visualization_pair.first.ClearData();
}

/**
 *
 */
void IO::VisualizationManager::WriteToDisk(
    const double visualziation_time, const int visualization_step)
{
  for (auto& [key, visualization_pair] : visualization_map_)
  {
    visualization_pair.first.ConsistencyCheckAndCompleteData();
    visualization_pair.second->WriteVisualizationDataToDisk(
        visualization_pair.first, visualziation_time, visualization_step);
  }
}

/**
 *
 */
std::string IO::VisualizationManager::GetVisualizationDataNameForOutputFiles(
    const std::string& visualization_data_name) const
{
  if (visualization_data_name == "")
    return base_output_name_;
  else
    return base_output_name_ + "_" + visualization_data_name;
}

FOUR_C_NAMESPACE_CLOSE