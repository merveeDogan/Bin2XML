#include <stdio.h>

#include <string.h>

#include <stdlib.h>

#include <libxml/xmlschemastypes.h>

typedef struct record record;
typedef struct tag tag;
struct tag {
  char name[32];
  char surname[32];
  char gender[32];
  char email[32];
  char phone_number[32];
  char address[32];
  char level_of_education[32];
  char income_level[32];
  char expenditure[32];
  char currency_unit[32];
  char currentMood[32];
  char height[32];
  char weight[32];
};
tag tg;

struct record {
  char name[64]; //utf16
  char surname[32]; //utf8
  char gender;
  char email[32];
  char phone_number[16];
  char address[32];
  char level_of_education[8];
  unsigned int income_level; // given little-endian
  unsigned int expenditure; // given big-endian
  char currency_unit[16];
  char currentMood[32];
  float height;
  unsigned int weight;
};
record rc;

unsigned int BigEndian(unsigned int value) { //Turns BigEndian to LitteEndian or vice-versa
  unsigned int byte[4];
  unsigned int reversedValue;
  byte[0] = (value & 0x000000ff) << 24u; //lsb 1-8 bit -> msb 1-8 bit
  byte[1] = (value & 0x0000ff00) << 8u; //lsb 9-16 -> msb 9-16 bit
  byte[2] = (value & 0x00ff0000) >> 8u;//msb 9-16 -> lsb 9-16 bit
  byte[3] = (value & 0xff000000) >> 24u; //msb 1-8 bit -> lsb 1-8 bit
  reversedValue = byte[0] | byte[1] | byte[2] | byte[3]; //putting together
  return reversedValue;
};

char * toUtf8(u_int32_t character) { //Turns unicode into utf-8 (accepts one char from surname as 32 bit unsigned integer)
  char * encoded = malloc(4);
  if (character < (u_int32_t) 0x007F) {  //if char requires 1 bytes for encode
    encoded[0] = character;
    encoded = (char * ) realloc(encoded, 1);

  } else if (character < (u_int32_t) 0x07FF) {  //if char requires 2 bytes for encode
    encoded[1] = (character & 0x3f) | 0x80;
    encoded[0] = ((character & 0xfc0) >> 6u) | 0xC0;
    encoded = (char * ) realloc(encoded, 2);
  } else if (character < (u_int32_t) 0xFFFF) {  //if char requires 3 bytes for encode
    encoded[2] = (character & 0x3f) | 0x80;
    encoded[1] = ((character & 0xfc0) >> 6u) | 0x80;
    encoded[0] = ((character & 0x3f000) >> 12u) | 0xE0;
    encoded = (char * ) realloc(encoded, 3);
  } else {  //if char requires 4 bytes for encode
    encoded[3] = (character & 0x3f) | 0x80;
    encoded[2] = ((character & 0xfc0) >> 6u) | 0x80;
    encoded[1] = (character & 0x3f000) >> 12u | 0x80;
    encoded[0] = (character & 0xfc0000) >> 18u | 0xF0;

  }
  return encoded;
}
char * fromUtf8(char surname[]) { //Accepts surname as char array

  u_int8_t byte[4]; //for a 4 byte char
  u_int32_t character = 0x00000000;
  int len = strlen(surname);
  char * word = malloc(len); //dynamic memory allocation
  for (size_t i = 0; i < len; i++) //32byte surname
  {
    byte[0] = surname[i];
    byte[1] = surname[i + 1];
    byte[2] = surname[i + 2];
    byte[3] = surname[i + 3];

    if (byte[0] == (u_int8_t) 0x00)
      continue;
    if (byte[0] >= (u_int8_t) 0xF0) { //if char requires 4 bytes for encode
      byte[0] -= ((u_int8_t) 0xF0);
      byte[1] -= ((u_int8_t) 0x80);
      byte[2] -= ((u_int8_t) 0x80);
      byte[3] -= ((u_int8_t) 0x80);
      character = ((byte[0] << 18u) | (byte[1] << 12u) | (byte[2] << 6u) | byte[3]); //putting together
      strcat(word, toUtf8(character));
      i += 3;
    } else if (byte[0] >= (u_int8_t) 0xE0) { //if char requires 3 bytes for encode
      byte[0] -= ((u_int8_t) 0xE0);
      byte[1] -= ((u_int8_t) 0x80);
      byte[2] -= ((u_int8_t) 0x80);
      character = ((byte[0] << 12u) | (byte[1] << 6u) | byte[2]); //putting together
      strcat(word, toUtf8(character));
      i += 2;
    } else if (byte[0] >= (u_int8_t) 0xC0) {  //if char requires 2 bytes for encode
      byte[0] -= ((u_int8_t) 0xC0);
      byte[1] -= ((u_int8_t) 0x80);
      character = ((byte[0] << 6u) | byte[1]); //putting together
      strcat(word, toUtf8(character));
      i++;
    } else { // //if char requires 1 bytes for encode
      character = (u_int32_t) byte[0];
      strcat(word, toUtf8(character));
    }
  }
  return word;
}
char * toUtf16(u_int32_t character) { //Turns unicode into utf-16 (accepts one char from name as 32 bit unsigned integer)
  char * encoded = malloc(4);  //dynamic memory allocation
  if (character < (u_int32_t) 0x10000) {
    encoded[1] = (character & 0xff);
    encoded[0] = ((character & 0xff00) >> 8u);
    encoded = (char * ) realloc(encoded, 2); //shrinks char array to 2 char size
  } else {
    encoded[3] = (character & 0xff);
    encoded[2] = ((character & 0x300) >> 8u) | 0xDC;
    encoded[1] = ((character & 0x3fc00) - (0x40)) >> 10u;
    encoded[0] = (character & 0xC0000) >> 18u | 0xD8;
  }
  return encoded; //returns char array
}

char * fromUtf16(char name[]) { //Accepts name as char array

  char * word = malloc(64);

  u_int8_t leftByte; //left byte
  u_int8_t rightByte; //right byte
  u_int16_t codeUnit; //first 2 byte (leftByte|rightByte)

  u_int8_t leftByte2; //second left byte
  u_int8_t rightByte2; //second right byte
  u_int16_t codeUnit2; //second 2 byte (leftByte2|rightByte2)

  u_int32_t codeUnit3; //final value (codeUnit|codeUnit2)
  int i = 0;
  for (; name[i] != 0; i += 2) { //Loop untill end of name array
    leftByte = name[i];
    rightByte = name[i + 1];
    codeUnit = (leftByte << 8) | (rightByte);

    leftByte2 = name[i + 2];
    rightByte2 = name[i + 3];
    codeUnit2 = (leftByte2 << 8) | (rightByte2); //codeUnit2 & codeUnit3 is used when char has two code unit
	
    if (codeUnit >= (u_int16_t) 0xD800 && codeUnit2 >= (u_int16_t) 0xDC00) { //if char requires 4 bytes for encode
      codeUnit -= (u_int16_t) 0xD800;
      codeUnit2 -= (u_int16_t) 0xDC00;
      codeUnit *= (u_int16_t) 0x0400;
      codeUnit3 = (codeUnit << 10) | (codeUnit2);  //putting together
      codeUnit3 += (u_int32_t) 0x00010000;
      strcat(word, toUtf16(codeUnit3));
      i += 2;

    } else { //if char requires 2 bytes for encode
      strcat(word, toUtf16(codeUnit));
    }
  }
  word = (char * ) realloc(word, (i) * sizeof(char)); //trimming unnecessary bytes
  return word;
}

void toXML(record persons[], char destinationPath[], int count) { //Writes persons array to XML file
  int counter = 1;
  FILE * XMLfile = fopen(destinationPath, "w");
  fprintf(XMLfile, "<?xml version='1.0' encoding='utf-8'?>\n"); //start-tag
  fprintf(XMLfile, "<records>\n"); //records tag
  for (int i = 1; i <= count + 1; i++) {// null record checker
    if (!strcmp(persons[i].name, "")) { //if person's name is null
      continue;
    }

    fprintf(XMLfile, "\t<row id=\"%d\">\n", counter); //row tag
    //informations
    fprintf(XMLfile, "\t\t<%s>%s</%s>\n", tg.name, fromUtf16(persons[i].name), tg.name);
    fprintf(XMLfile, "\t\t<%s>%s</%s>\n", tg.surname, fromUtf8(persons[i].surname), tg.surname);
    fprintf(XMLfile, "\t\t<%s>%c</%s>\n", tg.gender, persons[i].gender, tg.gender);
    fprintf(XMLfile, "\t\t<%s>%s</%s>\n", tg.email, persons[i].email, tg.email);
    fprintf(XMLfile, "\t\t<%s>%s</%s>\n", tg.phone_number, persons[i].phone_number, tg.phone_number);
    fprintf(XMLfile, "\t\t<%s>%s</%s>\n", tg.address, persons[i].address, tg.address);
    fprintf(XMLfile, "\t\t<%s>%s</%s>\n", tg.level_of_education, persons[i].level_of_education, tg.level_of_education);
    fprintf(XMLfile, "\t\t<%s>%s</%s>\n", tg.currency_unit, persons[i].currency_unit, tg.currency_unit);
    fprintf(XMLfile, "\t\t<%s>%f</%s>\n", tg.height, persons[i].height, tg.height);
    fprintf(XMLfile, "\t\t<%s>%u</%s>\n", tg.weight, persons[i].weight, tg.weight);

    fprintf(XMLfile, "\t</row>\n");//close tag of row tag
    counter++;
  }
  fprintf(XMLfile, "</records>\n"); //close tag of records tag
  fclose(XMLfile);
};

int main(int argc, char ** argv) { //argc -> number of arguments, argv -> array of arguments

  if (argc <= 3) { //Check for the arguments
    printf("missing argument types\n");
    return 0;
  }
  record * persons;
  persons = (record * ) calloc(1, sizeof(record)); //structure for records (dynamically allocated)
  FILE * fp;
  fp = fopen(argv[1], "rb");
  if (fp == NULL) { //Path control
    puts("File can not be founded");
    exit(1);
  }
  //Reading and validating tags
  fread( & tg, sizeof(tg), 1, fp);
  !strcmp(tg.name, "name") ? (void) 0 : strcpy(tg.name, "name");
  !strcmp(tg.surname, "surname") ? (void) 0 : strcpy(tg.surname, "surname");
  !strcmp(tg.gender, "gender") ? (void) 0 : strcpy(tg.gender, "gender");
  !strcmp(tg.email, "email") ? (void) 0 : strcpy(tg.email, "email");
  !strcmp(tg.phone_number, "phone_number") ? (void) 0 : strcpy(tg.phone_number, "phone_number");
  !strcmp(tg.address, "address") ? (void) 0 : strcpy(tg.address, "address");
  !strcmp(tg.level_of_education, "level_of_education") ? (void) 0 : strcpy(tg.level_of_education, "level_of_education");
  !strcmp(tg.income_level, "income_level") ? (void) 0 : strcpy(tg.income_level, "income_level");
  !strcmp(tg.expenditure, "expenditure") ? (void) 0 : strcpy(tg.expenditure, "expenditure");
  !strcmp(tg.currency_unit, "currency_unit") ? (void) 0 : strcpy(tg.currency_unit, "currency_unit");
  !strcmp(tg.currentMood, "currentMood") ? (void) 0 : strcpy(tg.currentMood, "currentMood");
  !strcmp(tg.height, "height") ? (void) 0 : strcpy(tg.height, "height");
  !strcmp(tg.weight, "weight") ? (void) 0 : strcpy(tg.weight, "weight");
  fclose(fp);
  fp = fopen(argv[1], "rb");
  int counter = 0; //to count number of records
  while (!feof(fp)) { //checks if the file pointer has reached the end of file
    persons = (record * ) realloc(persons, (counter + 1) * sizeof(record)); //reallocate in every loop
    fread( & persons[counter], sizeof(record), 1, fp); //reads records from file
    counter++;
  }
  fclose(fp);
  toXML(persons, argv[2], counter);

  char * XMLFileName = argv[2]; //path of XML
  char * XSDFileName = argv[3]; //path of XSD
  xmlDocPtr document;
  xmlSchemaPtr schema = NULL;
  xmlSchemaParserCtxtPtr c_txt;
  xmlLineNumbersDefault(1);
  int ret;
  c_txt = xmlSchemaNewParserCtxt(XSDFileName);

  xmlSchemaSetParserErrors(c_txt, (xmlSchemaValidityErrorFunc) fprintf, (xmlSchemaValidityWarningFunc) fprintf, stderr);
  document = xmlReadFile(XMLFileName, NULL, 0);
  schema = xmlSchemaParse(c_txt);
  xmlSchemaFreeParserCtxt(c_txt);

  if (document == NULL) {
    fprintf(stderr, "Could not parse %s\n", XMLFileName);
  } else {
    xmlSchemaValidCtxtPtr c_txt;

    c_txt = xmlSchemaNewValidCtxt(schema);
    xmlSchemaSetValidErrors(c_txt, (xmlSchemaValidityErrorFunc) fprintf, (xmlSchemaValidityWarningFunc) fprintf, stderr);
    ret = xmlSchemaValidateDoc(c_txt, document);

    if (ret == 0) {
      printf("%s Validation Completed\n", XMLFileName);
    } else if (ret > 0) {
      printf("%s Validation Error\n", XMLFileName);
    } else {
      printf("%s Validation generated an internal error\n", XMLFileName);
    }
    xmlSchemaFreeValidCtxt(c_txt);
    xmlFreeDoc(document);
  }

  if (schema != NULL)
    xmlSchemaFree(schema);

  xmlSchemaCleanupTypes();
  xmlCleanupParser();
  xmlMemoryDump();
  return 0;

}