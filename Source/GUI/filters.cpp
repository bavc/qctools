#include "filters.h"
#include <string.h>

int countFilters() {

    int filtersCount = 0;
    while (strcmp(Filters[filtersCount].Name, "(End)"))
        filtersCount++;

    return filtersCount;
}

int FiltersListDefault_Count = countFilters();
