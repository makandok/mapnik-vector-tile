#ifndef MAPNIK_VECTOR_TILE_STRATEGY_HPP
#define MAPNIK_VECTOR_TILE_STRATEGY_HPP

// mapnik
#include <mapnik/config.hpp>
#include <mapnik/util/noncopyable.hpp>
#include <mapnik/proj_transform.hpp>
#include <mapnik/view_transform.hpp>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wunused-local-typedef"
#include <boost/geometry/core/coordinate_type.hpp>
#include <boost/geometry/core/access.hpp>
#include <boost/numeric/conversion/cast.hpp>
#pragma GCC diagnostic pop


namespace mapnik {

namespace vector_tile_impl {

struct vector_tile_strategy
{
    vector_tile_strategy(proj_transform const& prj_trans,
                         view_transform const& tr,
                         double scaling)
        : prj_trans_(prj_trans),
          tr_(tr),
          scaling_(scaling),
          not_equal_(!prj_trans_.equal()) {}

    template <typename P1, typename P2>
    inline bool apply(P1 const& p1, P2 & p2) const
    {
        using p2_type = typename boost::geometry::coordinate_type<P2>::type;
        double x = boost::geometry::get<0>(p1);
        double y = boost::geometry::get<1>(p1);
        double z = 0.0;
        if (not_equal_ && !prj_trans_.backward(x, y, z)) return false;
        tr_.forward(&x,&y);
        boost::geometry::set<0>(p2, static_cast<p2_type>(std::round(x * scaling_)));
        boost::geometry::set<1>(p2, static_cast<p2_type>(std::round(y * scaling_)));
        return true;
    }
    
    template <typename P1, typename P2>
    inline P2 execute(P1 const& p1, bool & status) const
    {
        P2 p2;
        status = apply(p1, p2);
        return p2;
    }

    proj_transform const& prj_trans_;
    view_transform const& tr_;
    double const scaling_;
    bool not_equal_;
};

// TODO - avoid creating degenerate polygons when first/last point of ring is skipped
struct transform_visitor {

    transform_visitor(vector_tile_strategy const& tr) :
      tr_(tr) {}

    inline mapnik::geometry::geometry<std::int64_t> operator() (mapnik::geometry::point<double> const& geom)
    {
        mapnik::geometry::point<std::int64_t> new_geom;
        if (!tr_.apply(geom,new_geom)) return mapnik::geometry::geometry_empty();
        return new_geom;
    }

    inline mapnik::geometry::geometry<std::int64_t> operator() (mapnik::geometry::multi_point<double> const& geom)
    {
        mapnik::geometry::multi_point<std::int64_t> new_geom;
        new_geom.reserve(geom.size());
        for (auto const& pt : geom)
        {
            mapnik::geometry::point<std::int64_t> pt2;
            if (tr_.apply(pt,pt2))
            {
                new_geom.push_back(std::move(pt2));
            }
        }
        return new_geom;
    }

    inline mapnik::geometry::geometry<std::int64_t> operator() (mapnik::geometry::line_string<double> const& geom)
    {
        mapnik::geometry::line_string<std::int64_t> new_geom;
        new_geom.reserve(geom.size());
        for (auto const& pt : geom)
        {
            mapnik::geometry::point<std::int64_t> pt2;
            if (tr_.apply(pt,pt2))
            {
                new_geom.push_back(std::move(pt2));
            }
        }
        return new_geom;
    }

    inline mapnik::geometry::geometry<std::int64_t> operator() (mapnik::geometry::multi_line_string<double> const& geom)
    {
        mapnik::geometry::multi_line_string<std::int64_t> new_geom;
        new_geom.reserve(geom.size());
        for (auto const& line : geom)
        {
            mapnik::geometry::line_string<std::int64_t> new_line;
            new_line.reserve(line.size());
            for (auto const& pt : line)
            {
                mapnik::geometry::point<std::int64_t> pt2;
                if (tr_.apply(pt,pt2))
                {
                    new_line.push_back(std::move(pt2));
                }
            }
            new_geom.push_back(std::move(new_line));
        }
        return new_geom;
    }

    inline mapnik::geometry::geometry<std::int64_t> operator() (mapnik::geometry::polygon<double> const& geom)
    {
        mapnik::geometry::polygon<std::int64_t> new_geom;
        new_geom.exterior_ring.reserve(geom.exterior_ring.size());
        for (auto const& pt : geom.exterior_ring)
        {
            mapnik::geometry::point<std::int64_t> pt2;
            if (tr_.apply(pt,pt2))
            {
                new_geom.exterior_ring.push_back(std::move(pt2));
            }
        }
        for (auto const& ring : geom.interior_rings)
        {
            mapnik::geometry::linear_ring<std::int64_t> new_ring;
            new_ring.reserve(ring.size());
            for (auto const& pt : ring)
            {
                mapnik::geometry::point<std::int64_t> pt2;
                if (tr_.apply(pt,pt2))
                {
                    new_ring.push_back(std::move(pt2));
                }
            }
            new_geom.interior_rings.push_back(std::move(new_ring));
        }
        return new_geom;
    }

    inline mapnik::geometry::geometry<std::int64_t> operator() (mapnik::geometry::multi_polygon<double> const& geom)
    {
        mapnik::geometry::multi_polygon<std::int64_t> new_geom;
        new_geom.reserve(geom.size());
        for (auto const& poly : geom)
        {
            mapnik::geometry::polygon<std::int64_t> new_poly;
            new_poly.exterior_ring.reserve(poly.exterior_ring.size());
            for (auto const& pt : poly.exterior_ring)
            {
                mapnik::geometry::point<std::int64_t> pt2;
                if (tr_.apply(pt,pt2))
                {
                    new_poly.exterior_ring.push_back(std::move(pt2));
                }
            }
            for (auto const& ring : poly.interior_rings)
            {
                mapnik::geometry::linear_ring<std::int64_t> new_ring;
                new_ring.reserve(ring.size());
                for (auto const& pt : ring)
                {
                    mapnik::geometry::point<std::int64_t> pt2;
                    if (tr_.apply(pt,pt2))
                    {
                        new_ring.push_back(std::move(pt2));
                    }
                }
                new_poly.interior_rings.push_back(std::move(new_ring));
            }
            new_geom.push_back(std::move(new_poly));
        }
        return new_geom;
    }

    inline mapnik::geometry::geometry<std::int64_t> operator() (mapnik::geometry::geometry_collection<double> const& geom)
    {
        mapnik::geometry::geometry_collection<std::int64_t> new_geom;
        new_geom.reserve(geom.size());
        for (auto const& g : geom)
        {
            new_geom.push_back(mapnik::util::apply_visitor((*this), g));
        }
        return new_geom;
     }

    inline mapnik::geometry::geometry<std::int64_t> operator() (mapnik::geometry::geometry_empty const& geom)
    {
        return geom;
    }
    vector_tile_strategy const& tr_;
};

}
}

#endif // MAPNIK_VECTOR_TILE_STRATEGY_HPP
