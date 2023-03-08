#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "paginator.h"
#include "read_input_functions.h"
#include "string_processing.h"
#include "document.h"
#include "search_server.h"
#include "request_queue.h"

using namespace std;

// -- Код поискового сервера

// -- Фреймвор для тестирования

void AssertImpl(const bool expr, const string &expr_str, const string &file,
                const string &func, const unsigned line,
                const string &hint = ""s)
{
  if (!expr)
  {
    cerr << file << "("s << line << "): "s << func << ": "s;
    cerr << "ASSERT("s << expr_str << ") failed."s;
    if (!hint.empty())
    {
      cerr << " Hint: "s << hint;
    }
    cerr << endl;
    abort();
  }
}

// Операторы вывода коллекций в консоль, чтобы можно было сравнивать коллекции в
// ASSERT_EQUAL
template <typename T>
ostream &operator<<(ostream &os, const vector<T> value)
{
  os << '[';
  for (size_t i = 0; i < value.size(); ++i)
  {
    if (i > 0)
    {
      os << ", "s;
    }
    os << value.at(i);
  }
  os << ']';
  return os;
}

template <typename T>
ostream &operator<<(ostream &os, const set<T> value)
{
  os << '{';
  size_t i = 0;
  for (const T &el : value)
  {
    if (i++ > 0)
    {
      os << ", "s;
    }
    os << el;
  }
  os << '}';
  return os;
}

template <typename K, typename V>
ostream &operator<<(ostream &os, const map<K, V> value)
{
  os << '{';
  size_t i = 0;
  for (const auto &[k, v] : value)
  {
    if (i++ > 0)
    {
      os << ", "s;
    }
    os << k << ": "s << v;
  }
  os << '}';
  return os;
}

#define ASSERT(expr) AssertImpl((expr), #expr, __FILE__, __FUNCTION__, __LINE__)
#define ASSERT_HINT(expr, hint) \
  AssertImpl((expr), #expr, __FILE__, __FUNCTION__, __LINE__, (hint))

template <typename T, typename U>
void AssertEqualImpl(const T &t, const U &u, const string &t_str,
                     const string &u_str, const string &file,
                     const string &func, const unsigned line,
                     const string &hint = ""s)
{
  if (t != u)
  {
    cerr << boolalpha;
    cerr << file << "("s << line << "): "s << func << ": "s;
    cerr << "ASSERT_EQUAL("s << t_str << ", "s << u_str << ") failed: "s;
    cerr << t << " != "s << u << "."s;
    if (!hint.empty())
    {
      cerr << " Hint: "s << hint;
    }
    cerr << endl;
    abort();
  }
}

#define ASSERT_EQUAL(a, b) \
  AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__)

#define ASSERT_EQUAL_HINT(a, b, hint) \
  AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))

#define ASSERT_CODE \
  try               \
  {
#define THROWS(exception_type)                                               \
  cerr << __FILE__ << "("s << __LINE__ << "): "s << __FUNCTION__ << ": "s;   \
  cerr << "ASSERT_CODE_THROWS failed: The code should have thrown '"s        \
       << #exception_type << "', but it didn't: "s << endl;                  \
  abort();                                                                   \
  }                                                                          \
  catch (const exception_type &ex)                                           \
  {                                                                          \
  }                                                                          \
  catch (...)                                                                \
  {                                                                          \
    cerr << __FILE__ << "("s << __LINE__ << "): "s << __FUNCTION__ << ": "s; \
    cerr << "ASSERT_CODE_THROWS failed: Throwed object is not of type '"s    \
         << #exception_type << "'"s << endl;                                 \
    abort();                                                                 \
  }

template <typename F>
void RunTestImpl(const F &func, const string &name)
{
  func();
  cerr << name << " OK"s << endl;
}

#define RUN_TEST(func) RunTestImpl((func), #func);

// -------- Начало модульных тестов поисковой системы ----------

// Тест проверяет, что поисковая система исключает стоп-слова при добавлении
// документов
void TestExcludeStopWordsFromAddedDocumentContent()
{
  const int doc_id = 42;
  const string content = "cat in the city"s;
  const vector<int> ratings = {1, 2, 3};
  {
    SearchServer server;
    server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
    const auto found_docs = server.FindTopDocuments("in"s);
    ASSERT_EQUAL(found_docs.size(), 1u);
    const Document &doc0 = found_docs[0];
    ASSERT_EQUAL(doc0.id, doc_id);
  }

  {
    SearchServer server("in the"s);
    server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
    ASSERT_HINT(server.FindTopDocuments("in"s).empty(),
                "Stop words must be excluded from documents"s);
  }
}

// Тест проверяет, что добавление документа увеличивает количество документов на
// сервере
void TestAddDocument()
{
  SearchServer server;
  ASSERT_EQUAL_HINT(server.GetDocumentCount(), 0,
                    "New SearchServer must be empty"s);

  server.AddDocument(0, "asd"s, DocumentStatus::ACTUAL, {});
  ASSERT_EQUAL_HINT(server.GetDocumentCount(), 1,
                    "Server must contain one added document"s);

  server.AddDocument(1, "qwe"s, DocumentStatus::ACTUAL, {});
  ASSERT_EQUAL_HINT(server.GetDocumentCount(), 2,
                    "Server must contain two added documents"s);
}

// Тест проверяет, что добавленные документы можно найти по запросу
void TestAddedDocumentCanBeFound()
{
  SearchServer server;
  vector<Document> documents = server.FindTopDocuments("asd"s);
  ASSERT_HINT(documents.empty(),
              "Server cannot find document if it was not added"s);

  server.AddDocument(0, "asd dsa"s, DocumentStatus::ACTUAL, {1, 2, 3});
  documents = server.FindTopDocuments("asd"s);
  ASSERT_EQUAL_HINT(documents.size(), 1, "Only one doc must be found"s);
  ASSERT_EQUAL_HINT(documents.at(0).id, 0,
                    "Added and found docs must have same ID"s);

  server.AddDocument(1, "zxc cxz"s, DocumentStatus::ACTUAL, {1, 2, 3});
  documents = server.FindTopDocuments("zxc"s);
  ASSERT_EQUAL_HINT(documents.size(), 1,
                    "There must be only one doc found by quety 'zxc'"s);
  ASSERT_EQUAL_HINT(documents.at(0).id, 1,
                    "Document found by query 'zxc' must have ID 1"s);

  server.AddDocument(2, "zxc cxz asd"s, DocumentStatus::ACTUAL, {});
  documents = server.FindTopDocuments("zxc asd"s);
  ASSERT_EQUAL_HINT(documents.size(), 3, "All 3 added documents must be found");
}

// Тест проверяет, что минус-слово в запросе исключает из выдачи документы,
// которые содеражат такое слово
void TestExcludeDocumentsWithMinusWordsFromSearchResult()
{
  SearchServer server("in the"s);

  server.AddDocument(0, "cat in the city"s, DocumentStatus::ACTUAL, {1, 2, 3});
  server.AddDocument(1, "cat in the garden"s, DocumentStatus::ACTUAL,
                     {1, 2, 3});

  // Одно минус-слово
  vector<Document> documents = server.FindTopDocuments("cat -city"s);
  ASSERT_EQUAL_HINT(documents.size(), 1,
                    "There must be only one document in the result"s);
  ASSERT_EQUAL_HINT(documents.at(0).id, 1,
                    "Document in the result must have ID '1'"s);

  // Два минус-слова
  ASSERT_HINT(server.FindTopDocuments("cat -garden -city").empty(),
              "These minus-words must exclude all documents from result"s);

  // Другой порядок обычных и минус-слов
  ASSERT_HINT(server.FindTopDocuments("-cat garden").empty(),
              "Minus-word 'cat' must exclude all documents from result"s);
}

// Тест проверяет, что метод MatchDocument возвращает именно те слова,
// которые пересекаются в документе и в запросе,
// и то, что если документ содержит минус-слово в из запроса, метод возвращает
// пустой вектор
void TestMatchDocumentReturnsExpectedWords()
{
  SearchServer server("in the"s);

  vector<string> expected_words;

  server.AddDocument(0, "cat in the city"s, DocumentStatus::ACTUAL, {1, 2, 3});
  const auto [match_words_1, status_1] = server.MatchDocument("cat"s, 0);
  expected_words = {"cat"s};
  ASSERT_EQUAL(match_words_1, expected_words);
  ASSERT_EQUAL(status_1, DocumentStatus::ACTUAL);

  const auto [match_words_2, status_2] = server.MatchDocument("cat city"s, 0);
  expected_words = {"cat"s, "city"s};
  ASSERT_EQUAL(match_words_2, expected_words);

  const auto [match_words_3, status_3] =
      server.MatchDocument("city some other words"s, 0);
  expected_words = {"city"s};
  ASSERT_EQUAL(match_words_3, expected_words);

  const auto [match_words_4, status_4] =
      server.MatchDocument("cat -city some other words"s, 0);
  ASSERT(match_words_4.empty());
}

void TestSortingByRelevanceAndByRating()
{
  // Проверяем, что выдача отсортирована по убыванию релевантности
  {
    SearchServer server("in the"s);

    server.AddDocument(0, "big cat in the city"s, DocumentStatus::ACTUAL, {1});
    server.AddDocument(1, "cat in the garden"s, DocumentStatus::ACTUAL, {1});
    server.AddDocument(2, "big black cat in the city"s, DocumentStatus::ACTUAL,
                       {1});
    server.AddDocument(3, "big cat"s, DocumentStatus::ACTUAL, {1});

    const vector<Document> docs =
        server.FindTopDocuments("big black cat city"s);
    ASSERT_EQUAL(docs.size(), 4);
    ASSERT(docs.at(0).relevance >= docs.at(1).relevance);
    ASSERT(docs.at(1).relevance >= docs.at(2).relevance);
    ASSERT(docs.at(2).relevance >= docs.at(3).relevance);
  }

  // Проверяем, что при равенстве релевантности выдача сортируется по убыванию
  // рейтинга
  {
    SearchServer server;
    server.AddDocument(0, "black cat"s, DocumentStatus::ACTUAL, {1});
    server.AddDocument(1, "red cat"s, DocumentStatus::ACTUAL, {2});
    server.AddDocument(2, "yellow cat"s, DocumentStatus::ACTUAL, {3});
    server.AddDocument(3, "gray cat"s, DocumentStatus::ACTUAL, {4});
    const vector<Document> documents = server.FindTopDocuments("cat"s);
    ASSERT_EQUAL(documents.size(), 4);
    ASSERT(abs(documents.at(0).relevance - documents.at(1).relevance) <
           EPSILON);
    ASSERT(documents.at(0).rating > documents.at(1).rating);
    ASSERT(abs(documents.at(1).relevance - documents.at(2).relevance) <
           EPSILON);
    ASSERT(documents.at(1).rating > documents.at(2).rating);
    ASSERT(abs(documents.at(2).relevance - documents.at(3).relevance) <
           EPSILON);
    ASSERT(documents.at(2).rating > documents.at(3).rating);
  }
}

// Проверяем расчёт рейтинга
void TestCalculateAverageRating()
{
  SearchServer server;

  server.AddDocument(1, "1"s, DocumentStatus::ACTUAL, {1, 2, 3, 4});
  vector<Document> documents = server.FindTopDocuments("1"s);
  ASSERT_EQUAL(documents.size(), 1);
  ASSERT_EQUAL(documents.at(0).rating, (1 + 2 + 3 + 4) / 4);

  server.AddDocument(2, "2"s, DocumentStatus::ACTUAL, {});
  documents = server.FindTopDocuments("2"s);
  ASSERT_EQUAL(documents.size(), 1);
  ASSERT_EQUAL(documents.at(0).rating, 0);

  server.AddDocument(3, "3"s, DocumentStatus::ACTUAL, {3, 5});
  documents = server.FindTopDocuments("3"s);
  ASSERT_EQUAL(documents.size(), 1);
  ASSERT_EQUAL(documents.at(0).rating, 4);

  server.AddDocument(4, "4"s, DocumentStatus::ACTUAL, {0});
  documents = server.FindTopDocuments("4"s);
  ASSERT_EQUAL(documents.size(), 1);
  ASSERT_EQUAL(documents.at(0).rating, 0);

  server.AddDocument(5, "5"s, DocumentStatus::ACTUAL, {-1, -5});
  documents = server.FindTopDocuments("5"s);
  ASSERT_EQUAL(documents.size(), 1);
  ASSERT_EQUAL(documents.at(0).rating, -3);

  server.AddDocument(6, "6"s, DocumentStatus::ACTUAL, {-1, 1});
  documents = server.FindTopDocuments("6"s);
  ASSERT_EQUAL(documents.size(), 1);
  ASSERT_EQUAL(documents.at(0).rating, 0);

  server.AddDocument(7, "7"s, DocumentStatus::ACTUAL, {-10, 0, 10, 20});
  documents = server.FindTopDocuments("7"s);
  ASSERT_EQUAL(documents.size(), 1);
  ASSERT_EQUAL(documents.at(0).rating, 5);
}

// Проверяем фильтрацию выдачи с помощью предиката,
void TestFilterResultByPredicate()
{
  SearchServer server;

  // по значению рейтинга
  server.AddDocument(0, "qwe ewq"s, DocumentStatus::ACTUAL, {});
  vector<Document> documents = server.FindTopDocuments(
      "qwe"s, [](const auto &_, const auto &__, const int rating)
      { return rating > 0; });
  ASSERT_HINT(documents.empty(), "Documents with rating > 0 must be filtered"s);

  // по значению статуса
  server.AddDocument(1, "qwe asd zxc"s, DocumentStatus::BANNED, {2});
  documents = server.FindTopDocuments(
      "qwe"s, [](const int _, const auto &status, const auto &__)
      { return status == DocumentStatus::ACTUAL; });
  ASSERT_EQUAL(documents.size(), 1);

  // по значению id документа
  documents = server.FindTopDocuments(
      "qwe"s,
      [](const int id, const auto &_, const auto &__)
      { return id == 1; });
  ASSERT_EQUAL(documents.size(), 1);
  ASSERT_EQUAL(documents.at(0).id, 1);
}

// Проверяем фильтрацию выдачи по статусу
void TestFilterResultByStatus()
{
  {
    SearchServer server("in the"s);

    server.AddDocument(0, "black cat"s, DocumentStatus::ACTUAL, {1, 2, 3});
    server.AddDocument(1, "cat in the city"s, DocumentStatus::ACTUAL,
                       {1, 2, 3});
    server.AddDocument(2, "city black cat"s, DocumentStatus::BANNED, {1, 2, 3});

    // Все подходят по запросу, и только два по статусу
    ASSERT_EQUAL(server.FindTopDocuments("cat"s, DocumentStatus::ACTUAL).size(),
                 2);

    // Два подходят по запросу, но только один из них по статусу
    ASSERT_EQUAL(
        server.FindTopDocuments("black"s, DocumentStatus::ACTUAL).size(), 1);

    // Все подходят по запросу, но только один по статусу
    ASSERT_EQUAL(server.FindTopDocuments("cat"s, DocumentStatus::BANNED).size(),
                 1);

    // Все подходят по запросу, но никто не подходит по статусу
    ASSERT(server.FindTopDocuments("cat"s, DocumentStatus::REMOVED).empty());
  }
}

// Проверяем расчёт релевантности
void TestCalculateDocumentRelevance()
{
  SearchServer server;

  // Добавим два документа
  server.AddDocument(0, "qwe ewq dsa"s, DocumentStatus::ACTUAL, {1, 2, 3});
  server.AddDocument(1, "qwe asd zxc dsa"s, DocumentStatus::ACTUAL, {1, 2, 3});

  vector<Document> documents = server.FindTopDocuments("qwe dsa"s);
  // Рассчитаем IDF для слов запроса
  // 'qwe': встречается в двух документах -> log( 2 / 2 ) = 0
  // 'dsa': встречается в двух документах -> log( 2 / 2 ) = 0
  // IDF у обоих равен 0, поэтому считать TF для этих слов нет смысла
  // Релевантность обоих документов должна равняться нулю

  ASSERT(documents.at(0).relevance < EPSILON);
  ASSERT(documents.at(1).relevance < EPSILON);

  documents = server.FindTopDocuments("qwe asd"s);
  // Рассчитаем IDF для слов запроса
  // 'qwe': встречается в двух документах -> log( 2 / 2 ) = 0
  // 'asd': встречается только во документе с ID '1' -> log( 2 / 1 ) ~= 0.693147
  // Посчитаем TF для слова 'asd' в документах
  // 1) не встречается в документе с ID '0' -> 0
  // 2) встречается один раз в документе с ID '1' из четырёх слов -> 1 / 4 =
  // 0.25 Релевантность документа с ID '0': 0 Релевантность документа с ID '1':
  // 0 + 0.693147 * 0.25 ~= 0.17328675 Документ с большей релевантностью должен
  // стоять на первом месте
  ASSERT(abs(documents.at(0).relevance - 0.17328675) < EPSILON);
  ASSERT(documents.at(1).relevance < EPSILON);
}

void TestAddDocumentThrowsExceptionWhenIdIdNegative()
{
  SearchServer server;

  ASSERT_CODE
  server.AddDocument(-1, "alksdf fdlkak"s, DocumentStatus::ACTUAL, {});
  THROWS(invalid_argument)
}

void TestAddDocumentThrowsExceptionWhenIdExists()
{
  SearchServer server;
  server.AddDocument(1, "asd"s, DocumentStatus::ACTUAL, {});

  ASSERT_CODE
  server.AddDocument(1, "kashdf ksjdahf"s, DocumentStatus::ACTUAL, {});
  THROWS(invalid_argument)
}

void TestAddDocumentThrowsExceptionIfDocumentContainsInvalidWords()
{
  SearchServer server;
  server.AddDocument(0, "asdf ak-sjf lasdk"s, DocumentStatus::ACTUAL, {});

  ASSERT_CODE
  server.AddDocument(1, "asdf - lasdk"s, DocumentStatus::ACTUAL, {});
  THROWS(invalid_argument)

  ASSERT_CODE
  server.AddDocument(2, "asdf -aksjf lasdk"s, DocumentStatus::ACTUAL, {});
  THROWS(invalid_argument)

  ASSERT_CODE
  server.AddDocument(3, "asdf aksjf- lasdk"s, DocumentStatus::ACTUAL, {});
  THROWS(invalid_argument)

  ASSERT_CODE
  server.AddDocument(4, "awk lfe\0as ldf"s, DocumentStatus::ACTUAL, {});
  THROWS(invalid_argument)
}

void TestSearchServerConstructorThrowsExceptionIfStopWordIsInvalid()
{
  ASSERT_CODE
  SearchServer server("alkak\0lsd asdf"s);
  THROWS(invalid_argument)
}

void TestFindAllDocumentsThrowsExceptionIfQueryContainsInvalidWords()
{
  SearchServer server;
  server.AddDocument(0, "asd"s, DocumentStatus::ACTUAL, {});

  ASSERT_CODE
  server.FindTopDocuments("asdf --asd"s);
  THROWS(invalid_argument)

  ASSERT_CODE
  server.FindTopDocuments("asdf -"s);
  THROWS(invalid_argument)

  ASSERT_CODE
  server.FindTopDocuments("asdf ajwfe-"s);
  THROWS(invalid_argument)

  ASSERT_CODE
  server.FindTopDocuments("asdf ajw\0fe"s);
  THROWS(invalid_argument)
}

void TestMatchDocumentThrowsExceptionIfQueryContainsInvalidWords()
{
  SearchServer server;
  server.AddDocument(0, "asd"s, DocumentStatus::ACTUAL, {});

  ASSERT_CODE
  server.MatchDocument("asdf --asd"s, 0);
  THROWS(invalid_argument)

  ASSERT_CODE
  server.MatchDocument("asdf -"s, 0);
  THROWS(invalid_argument)

  ASSERT_CODE
  server.MatchDocument("asdf ajwfe-"s, 0);
  THROWS(invalid_argument)

  ASSERT_CODE
  server.MatchDocument("asdf ajw\0fe"s, 0);
  THROWS(invalid_argument)
}

void TestGetDocumentIndexCanThrowsOutOfRangeException()
{
  SearchServer server;
  server.AddDocument(0, "asd"s, DocumentStatus::ACTUAL, {});

  ASSERT_CODE
  server.GetDocumentId(-1);
  THROWS(out_of_range);

  ASSERT_CODE
  server.GetDocumentId(23);
  THROWS(out_of_range);
}

void TestPaginateContainer()
{
  vector<int> container{1, 2, 3, 4, 5};
  const auto pages = Paginate(container, 2);

  // Должно получиться 3 страницы
  // Первая и вторая должны содержать по два элемента,
  // третья должна содержать один элемент

  // Проверяем первую страницу
  auto it = pages.begin();
  auto element_it = it->begin();

  ASSERT_EQUAL(1, *element_it);
  ++element_it;
  ASSERT_EQUAL(2, *element_it);
  ++element_it;
  ASSERT(element_it == it->end());

  // Проверяем вторую страницу
  ++it;
  element_it = it->begin();

  ASSERT_EQUAL(3, *element_it);
  ++element_it;
  ASSERT_EQUAL(4, *element_it);
  ++element_it;
  ASSERT(it->end() == element_it);

  // Проверяем третью страницу
  ++it;
  element_it = it->begin();

  ASSERT_EQUAL(5, *element_it);
  ++element_it;
  ASSERT(it->end() == element_it);

  // Проверяем, что это была последняя страница
  ++it;
  ASSERT(pages.end() == it);
}

void TestAddDocumentToRequestQueue()
{
  SearchServer search_server;
  RequestQueue request_queue(search_server);

  search_server.AddDocument(1, "asd qwe", DocumentStatus::ACTUAL, {1, 2, 3});

  vector<Document> result = request_queue.AddFindRequest("asd");

  // Результат не пуст
  ASSERT(!result.empty());
  // и в очереди не появилось запросов с пустым результатом
  ASSERT_EQUAL(0, request_queue.GetNoResultRequests());

  result = request_queue.AddFindRequest("empty request");

  // Результат пуст
  ASSERT(result.empty());
  // и в очереди появился один запрос с пустым результатом
  ASSERT_EQUAL(1, request_queue.GetNoResultRequests());
}

void TestRemoveOldRequestsFromQueue()
{
  SearchServer search_server;
  RequestQueue request_queue(search_server);

  search_server.AddDocument(1, "valid", DocumentStatus::ACTUAL, {});
  
  for (int i = 0; i < 1440; ++i) {
    request_queue.AddFindRequest("emptyresult");
  }
  ASSERT_EQUAL(1440, request_queue.GetNoResultRequests());

  request_queue.AddFindRequest("valid");
  ASSERT_EQUAL(1439, request_queue.GetNoResultRequests());
}

// Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer()
{
  RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
  RUN_TEST(TestAddDocument);
  RUN_TEST(TestAddedDocumentCanBeFound);
  RUN_TEST(TestExcludeDocumentsWithMinusWordsFromSearchResult);
  RUN_TEST(TestMatchDocumentReturnsExpectedWords);
  RUN_TEST(TestSortingByRelevanceAndByRating);
  RUN_TEST(TestCalculateAverageRating);
  RUN_TEST(TestFilterResultByPredicate);
  RUN_TEST(TestFilterResultByStatus);
  RUN_TEST(TestCalculateDocumentRelevance);
  RUN_TEST(TestAddDocumentThrowsExceptionWhenIdIdNegative);
  RUN_TEST(TestAddDocumentThrowsExceptionWhenIdExists);
  RUN_TEST(TestAddDocumentThrowsExceptionIfDocumentContainsInvalidWords);
  RUN_TEST(TestSearchServerConstructorThrowsExceptionIfStopWordIsInvalid);
  RUN_TEST(TestFindAllDocumentsThrowsExceptionIfQueryContainsInvalidWords);
  RUN_TEST(TestGetDocumentIndexCanThrowsOutOfRangeException);
  RUN_TEST(TestPaginateContainer);
  RUN_TEST(TestRemoveOldRequestsFromQueue);
}

// --------- Окончание модульных тестов поисковой системы -----------

int main()
{
  SearchServer search_server("and in at"s);
  RequestQueue request_queue(search_server);
  search_server.AddDocument(1, "curly cat curly tail"s, DocumentStatus::ACTUAL, {7, 2, 7});
  search_server.AddDocument(2, "curly dog and fancy collar"s, DocumentStatus::ACTUAL, {1, 2, 3});
  search_server.AddDocument(3, "big cat fancy collar "s, DocumentStatus::ACTUAL, {1, 2, 8});
  search_server.AddDocument(4, "big dog sparrow Eugene"s, DocumentStatus::ACTUAL, {1, 3, 2});
  search_server.AddDocument(5, "big dog sparrow Vasiliy"s, DocumentStatus::ACTUAL, {1, 1, 1});
  // 1439 запросов с нулевым результатом
  for (int i = 0; i < 1439; ++i)
  {
    request_queue.AddFindRequest("empty request"s);
  }
  // все еще 1439 запросов с нулевым результатом
  request_queue.AddFindRequest("curly dog"s);
  // новые сутки, первый запрос удален, 1438 запросов с нулевым результатом
  request_queue.AddFindRequest("big collar"s);
  // первый запрос удален, 1437 запросов с нулевым результатом
  request_queue.AddFindRequest("sparrow"s);
  cout << "Total empty requests: "s << request_queue.GetNoResultRequests() << endl;

  TestSearchServer();
  return 0;
}
