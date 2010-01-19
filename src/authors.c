
/* NOTE: This file is auto-generated from authors.xml, do not edit it. */

#include "authors.h"

static const char * const authors[] =
{
        "Andrew Stribblehill",
        "Chris Lightfoot",
        "David Shanks",
        "George Talusan",
        "James E. Flemer",
        "Jeremy Hinegardner",
        "Kim Holviala",
        "Mathew Mrosko",
        "Matthew Dempsky",
        "Michael Adam",
        "Mukund Sivaraman",
        "Petr Lampa",
        "Robert James Kaes",
        "Steven Young",
        NULL
};

static const char * const documenters[] =
{
        "Marc Silver",
        "Michael Adam",
        "Mukund Sivaraman",
        "Robert James Kaes",
        "Steven Young",
        NULL
};

const char * const *
authors_get_authors (void)
{
        return authors;
}

const char * const *
authors_get_documenters (void)
{
        return documenters;
}
