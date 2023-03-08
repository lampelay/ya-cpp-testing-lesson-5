#include <ostream>
#include "document.h"

using namespace std;

// DocumentStatus

ostream &operator<<(ostream &os, const DocumentStatus &status)
{
  switch (status)
  {
  case DocumentStatus::ACTUAL:
    os << "ACTUAL"s;
    break;
  case DocumentStatus::BANNED:
    os << "BANNED"s;
    break;
  case DocumentStatus::IRRELEVANT:
    os << "IRRELEVANT"s;
    break;
  case DocumentStatus::REMOVED:
    os << "REMOVED"s;
    break;
  }
  return os;
}


// Document

Document::Document() = default;

Document::Document(int id, double relevance, int rating)
    : id(id), relevance(relevance), rating(rating) {}

ostream &operator<<(ostream &out, const Document &document)
{
  return out << "{ document_id = " << document.id
             << ", relevance = " << document.relevance
             << ", rating = " << document.rating << " }";
}