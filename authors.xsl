<?xml version="1.0" encoding="UTF-8"?>

<!--

   Simple XSL transformation to create a text version AUTHORS file from
   authors.xml. This file was adapted from GIMP.

-->

<xsl:stylesheet version="1.0"
                xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns:dc="http://purl.org/dc/elements/1.1/">

  <xsl:output method="text" />

  <xsl:template name="contributor">
    <xsl:param name="role" />
    <xsl:apply-templates select="dc:contributor[contains(@role, $role)]" />
  </xsl:template>

  <xsl:template match="/dc:authors">
    <xsl:text>Tinyproxy AUTHORS
=================

////
This file is generated from authors.xml, do not edit it directly.
////

Coding
------

The following people have contributed code to Tinyproxy:

</xsl:text>
    <xsl:call-template name="contributor">
      <xsl:with-param name="role" select="'author'"/>
    </xsl:call-template>
    <xsl:text>

Documentation
-------------

The following people have helped to document Tinyproxy:

</xsl:text>
    <xsl:call-template name="contributor">
      <xsl:with-param name="role" select="'documenter'"/>
    </xsl:call-template>

  </xsl:template>

  <xsl:template match="dc:contributor">
    <xsl:text> * </xsl:text><xsl:apply-templates /><xsl:text>
</xsl:text>
  </xsl:template>

</xsl:stylesheet>
