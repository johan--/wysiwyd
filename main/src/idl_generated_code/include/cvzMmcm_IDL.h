// This is an automatically-generated file.
// It could get re-generated if the ALLOW_IDL_GENERATION flag is on.

#ifndef YARP_THRIFT_GENERATOR_cvzMmcm_IDL
#define YARP_THRIFT_GENERATOR_cvzMmcm_IDL

#include <yarp/os/Wire.h>
#include <yarp/os/idl/WireTypes.h>

class cvzMmcm_IDL;


/**
 * cvzMmcm_IDL
 * IDL Interface to \ref cvz - mmcm services.
 */
class cvzMmcm_IDL : public yarp::os::Wire {
public:
  cvzMmcm_IDL() { yarp().setOwner(*this); }
/**
 * Set the learning rate.
 */
  virtual void setLearningRate(const double l);
/**
 * Get the current learning rate.
 */
  virtual double getLearningRate();
/**
 * Set the neighborhood radius (sigma).
 */
  virtual void setSigma(const double s);
/**
 * Get the current neighborhood radius (sigma).
 */
  virtual double getSigma();
/**
 * Get the activity of a neuron.
 * @param x the horizontal coordinate of the neuron.
 * @param y the vertical coordinate of the neuron.
 * @param z the layer to which the neuron belongs.
 * @return Current activity of the neuron.
 */
  virtual double getActivity(const int32_t x, const int32_t y, const int32_t z);
/**
 * Save the current weights of the map for latter reloading.
 * @param path The path to where you want to store the weights.
 * @return true/false in case of success/failure.
 */
  virtual bool saveWeightsTofile(const std::string& path);
/**
 * Load the weights of the map from a file.
 * The file should have been saved from a map using the same config file (modaility names & size, map size, etc.).
 * @param path Path to the file containing the weights.
 * @return true/false in case of success/failure.
 */
  virtual bool loadWeightsFromfile(const std::string& path);
  virtual bool read(yarp::os::ConnectionReader& connection);
  virtual std::vector<std::string> help(const std::string& functionName="--all");
};

#endif
