<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
  <xsl:import href="http://docbook.sourceforge.net/release/xsl/current/xhtml/docbook.xsl"/>

  <xsl:output indent="yes"/>

  <!-- Turn citerefentry elements into HTML links -->
  <xsl:param name="citerefentry.link" select="1"/>

  <!-- But not for those with role=nolink -->
  <xsl:template match="citerefentry[@role='nolink']">
    <xsl:call-template name="inline.charseq"/>
  </xsl:template>

  <!-- Code to generate the URL for a given citerefentry element -->
  <xsl:template name="generate.citerefentry.link">
    <xsl:value-of select="refentrytitle"/>
    <xsl:text>.html</xsl:text>
  </xsl:template>

</xsl:stylesheet>
