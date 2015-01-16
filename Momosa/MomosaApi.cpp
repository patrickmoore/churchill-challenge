#include "MomosaApi.hpp"
#include "SearchContext.hpp"


SearchContext* create(const Point* points_begin, const Point* points_end)
{
    SearchContext* context = new SearchContext(points_begin, points_end);
    return context;
}

int32_t search(SearchContext* sc, const Rect rect, const int32_t count, Point* out_points)
{
    return sc->search(rect, count, out_points);
}

SearchContext* destroy(SearchContext* sc)
{
    delete sc;
    return nullptr;
}