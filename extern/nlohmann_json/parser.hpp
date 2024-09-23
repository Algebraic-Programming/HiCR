#pragma once

/**
 * @file parser.hpp
 * @brief Contains common functions related to JSON manipulation
 * 
 * This file was adapted from https://github.com/SergioMartin86/jaffarCommon/blob/main/include/jaffarCommon/json.hpp
 * by Sergio Martin, MIT License
 * https://github.com/SergioMartin86/jaffarCommon
 */

#include <cstdlib>
#include <nlohmann_json/json.hpp>
#include <hicr/core/exceptions.hpp>

namespace hicr
{

namespace json
{

/**
 * Verifies the given key exists in the given json object
 *
 * @note An exception will occur if the input is not a json object (e.g., is an array)
 * @note An exception will occur if the given key does not exist in the json object
 *
 * @param[in] json The json object to look into
 * @param[in] key The key to look for inside the json object
 */
__INLINE__ const void checkEntry(const nlohmann::json &json, const std::string &key)
{
  if (json.is_object() == false)
    HICR_THROW_LOGIC("[Error] JSON passed is not a key/value object. Happened when trying to obtain string key '%s'. JSON Dump: %s\n", key.c_str(), json.dump(2).c_str());
  if (json.contains(key) == false) HICR_THROW_LOGIC("[Error] JSON contains no field called '%s'. JSON Dump: %s\n", key.c_str(), json.dump(2).c_str());
}

/**
 * Returns the string stored in the given key of the given json object
 *
 * @note An exception will occur if the input is not a json object (e.g., is an array)
 * @note An exception will occur if the given key does not exist in the json object
 * @note An exception will occur if the given key is not a string
 *
 * @param[in] json The json object to look into
 * @param[in] key The key to look for inside the json object
 * @return A string contained in the key
 */
__INLINE__ const std::string getString(const nlohmann::json &json, const std::string &key)
{
  checkEntry(json, key);
  if (json[key].is_string() == false) HICR_THROW_LOGIC("[Error] Configuration key '%s' is not a string. JSON Dump: %s\n", key.c_str(), json.dump(2).c_str());
  return json.at(key).get<std::string>();
}

/**
 * Returns the object stored in the given key of the given json object
 *
 * @note An exception will occur if the input is not a json object (e.g., is an array)
 * @note An exception will occur if the given key does not exist in the json object
 * @note An exception will occur if the given key is not an object
 *
 * @param[in] json The json object to look into
 * @param[in] key The key to look for inside the json object
 * @return A json object contained in the key
 */
__INLINE__ const nlohmann::json &getObject(const nlohmann::json &json, const std::string &key)
{
  checkEntry(json, key);
  if (json[key].is_object() == false) HICR_THROW_LOGIC("[Error] Configuration key '%s' is not a key/value object. JSON Dump: %s\n", key.c_str(), json.dump(2).c_str());
  return json.at(key);
}

/**
 * Returns the array of the specified type stored in the given key of the given json object
 *
 * @note An exception will occur if the input is not a json object (e.g., is an array)
 * @note An exception will occur if the given key does not exist in the json object
 * @note An exception will occur if the given key is not an array
 *
 * @param[in] json The json object to look into
 * @param[in] key The key to look for inside the json object
 * @return A vector of the type specified containing all the elements of the array of key entry
 */
template <typename T>
__INLINE__ const std::vector<T> getArray(const nlohmann::json &json, const std::string &key)
{
  checkEntry(json, key);
  if (json[key].is_array() == false) HICR_THROW_LOGIC("[Error] Configuration key '%s' is not an array. JSON Dump: %s\n", key.c_str(), json.dump(2).c_str());
  return json.at(key).get<std::vector<T>>();
}

/**
 * Returns the number of the specified type stored the given key of the given json object
 *
 * @note An exception will occur if the input is not a json object (e.g., is an array)
 * @note An exception will occur if the given key does not exist in the json object
 * @note An exception will occur if the given key is not a number
 *
 * @param[in] json The json object to look into
 * @param[in] key The key to look for inside the json object
 * @return The number stored in the key entry
 */
template <typename T>
__INLINE__ const T getNumber(const nlohmann::json &json, const std::string &key)
{
  checkEntry(json, key);
  if (json[key].is_number() == false) HICR_THROW_LOGIC("[Error] Configuration key '%s' is not a number. JSON Dump: %s\n", key.c_str(), json.dump(2).c_str());
  return json.at(key).get<T>();
}

/**
 * Returns the boolean stored the given key of the given json object
 *
 * @note An exception will occur if the input is not a json object (e.g., is an array)
 * @note An exception will occur if the given key does not exist in the json object
 * @note An exception will occur if the given key is not a boolean
 *
 * @param[in] json The json object to look into
 * @param[in] key The key to look for inside the json object
 * @return The boolean stored in the key entry
 */
__INLINE__ const bool getBoolean(const nlohmann::json &json, const std::string &key)
{
  checkEntry(json, key);
  if (json[key].is_boolean() == false) HICR_THROW_LOGIC("[Error] Configuration key '%s' is not a boolean. JSON Dump: %s\n", key.c_str(), json.dump(2).c_str());
  return json.at(key).get<bool>();
}

} // namespace hicr

} // namespace jaffarCommon