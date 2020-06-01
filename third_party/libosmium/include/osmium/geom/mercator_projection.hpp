#ifndef OSMIUM_GEOM_MERCATOR_PROJECTION_HPP
#define OSMIUM_GEOM_MERCATOR_PROJECTION_HPP

/*

This file is part of Osmium (https://osmcode.org/libosmium).

Copyright 2013-2019 Jochen Topf <jochen@topf.org> and others (see README).

Boost Software License - Version 1.0 - August 17th, 2003

Permission is hereby granted, free of charge, to any person or organization
obtaining a copy of the software and accompanying documentation covered by
this license (the "Software") to use, reproduce, display, distribute,
execute, and transmit the Software, and to prepare derivative works of the
Software, and to permit third-parties to whom the Software is furnished to
do so, all subject to the following:

The copyright notices in the Software and this entire statement, including
the above license grant, this restriction and the following disclaimer,
must be included in all copies of the Software, in whole or in part, and
all derivative works of the Software, unless such copies or derivative
works are solely in the form of machine-executable object code generated by
a source language processor.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.

*/

#include <osmium/geom/coordinates.hpp>
#include <osmium/geom/util.hpp>
#include <osmium/osm/location.hpp>

#include <cmath>
#include <string>

namespace osmium {

    namespace geom {

        namespace detail {

            constexpr double earth_radius_for_epsg3857 = 6378137.0;
            constexpr double max_coordinate_epsg3857 = 20037508.34;

            constexpr inline double lon_to_x(double lon) noexcept {
                return earth_radius_for_epsg3857 * deg_to_rad(lon);
            }

            inline double lat_to_y_with_tan(double lat) { // not constexpr because math functions aren't
                return earth_radius_for_epsg3857 * std::log(std::tan(osmium::geom::PI / 4 + deg_to_rad(lat) / 2));
            }

#ifdef OSMIUM_USE_SLOW_MERCATOR_PROJECTION
            inline double lat_to_y(double lat) {
                return lat_to_y_with_tan(lat);
            }
#else

            // This is a much faster implementation than the canonical
            // implementation using the tan() function. For details
            // see https://github.com/osmcode/mercator-projection .
            inline double lat_to_y(double lat) { // not constexpr because math functions aren't
                if (lat < -78.0 || lat > 78.0) {
                    return lat_to_y_with_tan(lat);
                }

                return earth_radius_for_epsg3857 *
                    ((((((((((-3.1112583378460085319e-23  * lat +
                               2.0465852743943268009e-19) * lat +
                               6.4905282018672673884e-18) * lat +
                              -1.9685447939983315591e-14) * lat +
                              -2.2022588158115104182e-13) * lat +
                               5.1617537365509453239e-10) * lat +
                               2.5380136069803016519e-9)  * lat +
                              -5.1448323697228488745e-6)  * lat +
                              -9.4888671473357768301e-6)  * lat +
                               1.7453292518154191887e-2)  * lat)
                    /
                    ((((((((((-1.9741136066814230637e-22  * lat +
                              -1.258514031244679556e-20)  * lat +
                               4.8141483273572351796e-17) * lat +
                               8.6876090870176172185e-16) * lat +
                              -2.3298743439377541768e-12) * lat +
                              -1.9300094785736130185e-11) * lat +
                               4.3251609106864178231e-8)  * lat +
                               1.7301944508516974048e-7)  * lat +
                              -3.4554675198786337842e-4)  * lat +
                              -5.4367203601085991108e-4)  * lat + 1.0);
            }
#endif

            constexpr inline double x_to_lon(double x) {
                return rad_to_deg(x) / earth_radius_for_epsg3857;
            }

            inline double y_to_lat(double y) { // not constexpr because math functions aren't
                return rad_to_deg(2 * std::atan(std::exp(y / earth_radius_for_epsg3857)) - osmium::geom::PI / 2);
            }

        } // namespace detail

        /**
         * The maximum latitude that can be projected with the Web Mercator
         * (EPSG:3857) projection.
         */
        constexpr double MERCATOR_MAX_LAT = 85.0511288;

        /**
         * Convert the coordinates from WGS84 lon/lat to web mercator.
         *
         * @pre @code c.valid() @endcode
         * @pre Coordinates must be in valid range, longitude between
         *      -180 and +180 degree, latitude between -MERCATOR_MAX_LAT
         *      and MERCATOR_MAX_LAT.
         */
        inline Coordinates lonlat_to_mercator(const Coordinates& c) {
            return Coordinates{detail::lon_to_x(c.x), detail::lat_to_y(c.y)};
        }

        /**
         * Convert the coordinates from web mercator to WGS84 lon/lat.
         *
         * @pre @code c.valid() @endcode
         * @pre Coordinates must be in valid range (longitude and
         *      latidude between -/+20037508.34).
         */
        inline Coordinates mercator_to_lonlat(const Coordinates& c) {
            return Coordinates{detail::x_to_lon(c.x), detail::y_to_lat(c.y)};
        }

        /**
         * Functor that does projection from WGS84 (EPSG:4326) to "Web
         * Mercator" (EPSG:3857)
         */
        class MercatorProjection {

        public:

            // This is not "= default" on purpose because some compilers don't
            // like it and complain that "default initialization of an object
            // of const type 'const osmium::geom::MercatorProjection' requires
            // a user-provided default constructor".
            MercatorProjection() { // NOLINT(hicpp-use-equals-default, modernize-use-equals-default)
            }

            /**
             * Do coordinate transformation.
             *
             * @pre Coordinates must be in valid range, longitude between
             *      -180 and +180 degree, latitude between -MERCATOR_MAX_LAT
             *      and MERCATOR_MAX_LAT.
             */
            Coordinates operator()(osmium::Location location) const {
                return Coordinates{detail::lon_to_x(location.lon()), detail::lat_to_y(location.lat())};
            }

            int epsg() const noexcept {
                return 3857;
            }

            std::string proj_string() const {
                return "+proj=merc +a=6378137 +b=6378137 +lat_ts=0.0 +lon_0=0.0 +x_0=0.0 +y_0=0 +k=1.0 +units=m +nadgrids=@null +wktext +no_defs";
            }

        }; // class MercatorProjection

    } // namespace geom

} // namespace osmium

#endif // OSMIUM_GEOM_MERCATOR_PROJECTION_HPP
