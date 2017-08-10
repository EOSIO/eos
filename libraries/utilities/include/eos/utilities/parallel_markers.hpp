#pragma once

#include <boost/range/combine.hpp>
#include <boost/range/adaptor/filtered.hpp>
#include <boost/range/adaptor/transformed.hpp>

namespace eos { namespace utilities {

/**
 * @brief Return values in DataRange corresponding to matching Markers
 *
 * Takes two parallel ranges, a Data range containing data values, and a Marker range containing markers on the
 * corresponding data values. Returns a new Data range containing only the values corresponding to markers which match
 * markerValue
 *
 * For example:
 * @code{.cpp}
 * vector<char> data = {'A', 'B', 'C'};
 * vector<bool> markers = {true, false, true};
 * auto markedData = FilterDataByMarker(data, markers, true);
 * // markedData contains {'A', 'C'}
 * @endcode
 */
template<typename DataRange, typename MarkerRange, typename Marker>
DataRange FilterDataByMarker(DataRange data, MarkerRange markers, const Marker& markerValue) {
   auto RemoveMismatchedMarkers = boost::adaptors::filtered([&markerValue](const auto& tuple) {
      return boost::get<0>(tuple) == markerValue;
   });
   auto ToData = boost::adaptors::transformed([](const auto& tuple) {
      return boost::get<1>(tuple);
   });

   // Zip the ranges together, filter out data with markers that don't match, and return the data without the markers
   auto range = boost::combine(markers, data) | RemoveMismatchedMarkers | ToData;
   return {range.begin(), range.end()};
}

}} // namespace eos::utilities

