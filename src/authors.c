
/* NOTE: This file is auto-generated from authors.xml, do not edit it. */

#include "authors.h"

static const char * const authors[] =
{
        "Andrew Stribblehill",
        "Chris Lightfoot",
        "Daniel Egger",
        "David Shanks",
        "Dmitry Semyonov",
        "George Talusan",
        "James E. Flemer",
        "Jeremy Hinegardner",
        "John van der Kamp",
        "Jordi Mallach",
        "Kim Holviala",
        "Mathew Mrosko",
        "Matthew Dempsky",
        "Michael Adam",
        "Moritz Muehlenhoff",
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
