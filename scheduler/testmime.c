/*
 * "$Id$"
 *
 *   MIME test program for the Common UNIX Printing System (CUPS).
 *
 *   Copyright 1997-2006 by Easy Software Products, all rights reserved.
 *
 *   These coded instructions, statements, and computer programs are the
 *   property of Easy Software Products and are protected by Federal
 *   copyright law.  Distribution and use rights are outlined in the file
 *   "LICENSE.txt" which should have been included with this file.  If this
 *   file is missing or damaged please contact Easy Software Products
 *   at:
 *
 *       Attn: CUPS Licensing Information
 *       Easy Software Products
 *       44141 Airport View Drive, Suite 204
 *       Hollywood, Maryland 20636 USA
 *
 *       Voice: (301) 373-9600
 *       EMail: cups-info@cups.org
 *         WWW: http://www.cups.org
 *
 * Contents:
 *
 *   main() - Main entry for the test program.
 */

/*
 * Include necessary headers...
 */

#include <stdio.h>
#include <stdlib.h>
#include <cups/string.h>
#include "mime.h"


/*
 * Local functions...
 */

static void print_rules(mime_magic_t *rules);


/*
 * 'main()' - Main entry for the test program.
 */

int					/* O - Exit status */
main(int  argc,				/* I - Number of command-line args */
     char *argv[])			/* I - Command-line arguments */
{
  int		i;			/* Looping vars */
  const char	*filter_path;		/* Filter path */
  char		super[MIME_MAX_SUPER],	/* Super-type name */
		type[MIME_MAX_TYPE];	/* Type name */
  int		compression;		/* Compression of file */
  int		cost;			/* Cost of filters */
  mime_t	*mime;			/* MIME database */
  mime_type_t	*src,			/* Source type */
		*dst;			/* Destination type */
  cups_array_t	*filters;		/* Filters for the file */
  mime_filter_t	*filter;		/* Current filter */


  mime        = NULL;
  src         = NULL;
  dst         = NULL;
  filter_path = "../filter:../pdftops";

  for (i = 1; i < argc; i ++)
    if (!strcmp(argv[i], "-d"))
    {
      i ++;

      if (i < argc)
        mime = mimeLoad(argv[i], filter_path);
    }
    else if (!strcmp(argv[i], "-f"))
    {
      i ++;

      if (i < argc)
        filter_path = argv[i];
    }
    else if (!src)
    {
      if (!mime)
	mime = mimeLoad("../conf", filter_path);

      src = mimeFileType(mime, argv[i], &compression);

      if (src)
	printf("%s: %s/%s%s\n", argv[i], src->super, src->type,
	       compression ? " (gzipped)" : "");
      else
      {
	printf("%s: unknown\n", argv[i]);
	if (mime)
	  mimeDelete(mime);
	return (1);
      }
    }
    else
    {
      sscanf(argv[i], "%15[^/]/%31s", super, type);
      dst = mimeType(mime, super, type);

      filters = mimeFilter(mime, src, dst, &cost, 10);

      if (!filters)
      {
	printf("No filters to convert from %s/%s to %s.\n", src->super,
	       src->type, argv[i]);
      }
      else
      {
        printf("Filter cost = %d\n", cost);

        filter = (mime_filter_t *)cupsArrayFirst(filters);
	fputs(filter->filter, stdout);

	for (filter = (mime_filter_t *)cupsArrayNext(filters);
	     filter;
	     filter = (mime_filter_t *)cupsArrayNext(filters))
	  printf(" | %s", filter->filter);

        putchar('\n');

        cupsArrayDelete(filters);
      }
    }

  if (!mime)
    mime = mimeLoad("../conf", filter_path);

  if (!src)
  {
    puts("MIME database types:");
    for (src = mimeFirstType(mime); src; src = mimeNextType(mime))
    {
      printf("\t%s/%s:\n", src->super, src->type);
      print_rules(src->rules);
      puts("");
    }

    puts("");

    puts("MIME database filters:");
    for (filter = mimeFirstFilter(mime); filter; filter = mimeNextFilter(mime))
      printf("\t%s/%s to %s/%s: %s (%d)\n",
             filter->src->super, filter->src->type,
	     filter->dst->super, filter->dst->type,
	     filter->filter, filter->cost);
  }

  return (0);
}


/*
 * 'print_rules()' - Print the rules for a file type...
 */

static void
print_rules(mime_magic_t *rules)	/* I - Rules to print */
{
  int	i;				/* Looping var */
  static char	indent[255] = "\t";	/* Indentation for rules */


  if (rules == NULL)
    return;

  while (rules != NULL)
  {
    printf("%s[%p] ", indent, rules);

    if (rules->invert)
      printf("NOT ");

    switch (rules->op)
    {
      case MIME_MAGIC_MATCH :
          printf("match(%s)", rules->value.matchv);
	  break;
      case MIME_MAGIC_LOCALE :
          printf("locale(%s)", rules->value.localev);
	  break;
      case MIME_MAGIC_ASCII :
          printf("ascii(%d,%d)", rules->offset, rules->length);
	  break;
      case MIME_MAGIC_PRINTABLE :
          printf("printable(%d,%d)", rules->offset, rules->length);
	  break;
      case MIME_MAGIC_STRING :
          printf("string(%d,", rules->offset);
	  for (i = 0; i < rules->length; i ++)
	    if (rules->value.stringv[i] < ' ' ||
	        rules->value.stringv[i] > 126)
	      printf("<%02X>", rules->value.stringv[i]);
	    else
	      putchar(rules->value.stringv[i]);
          putchar(')');
	  break;
      case MIME_MAGIC_CHAR :
          printf("char(%d,%d)", rules->offset, rules->value.charv);
	  break;
      case MIME_MAGIC_SHORT :
          printf("short(%d,%d)", rules->offset, rules->value.shortv);
	  break;
      case MIME_MAGIC_INT :
          printf("int(%d,%d)", rules->offset, rules->value.intv);
	  break;
      case MIME_MAGIC_CONTAINS :
          printf("contains(%d,%d,", rules->offset, rules->region);
	  for (i = 0; i < rules->length; i ++)
	    if (rules->value.stringv[i] < ' ' ||
	        rules->value.stringv[i] > 126)
	      printf("<%02X>", rules->value.stringv[i]);
	    else
	      putchar(rules->value.stringv[i]);
          putchar(')');
	  break;
      default :
	  break;
    }

    if (rules->child != NULL)
    {
      if (rules->op == MIME_MAGIC_OR)
	puts("OR (");
      else
	puts("AND (");

      strcat(indent, "\t");
      print_rules(rules->child);
      indent[strlen(indent) - 1] = '\0';
      printf("%s)\n", indent);
    }
    else
      putchar('\n');

    rules = rules->next;
  }
}


/*
 * End of "$Id$".
 */
