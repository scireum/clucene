/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#include "test.h"


/// Java PrefixQuery test, 2009-06-02
void testPrefixQuery(CuTest *tc){
	WhitespaceAnalyzer analyzer;
	RAMDirectory directory;
	const TCHAR* categories[] = {_T("/Computers"), _T("/Computers/Mac"), _T("/Computers/Windows")};

	IndexWriter writer( &directory, &analyzer, true);
	for (int i = 0; i < 3; i++) {
		Document *doc = _CLNEW Document();
		doc->add(*_CLNEW Field(_T("category"), categories[i], Field::STORE_YES | Field::INDEX_UNTOKENIZED));
		writer.addDocument(doc);
		_CLDELETE(doc);
	}
	writer.close();

	Term* t = _CLNEW Term(_T("category"), _T("/Computers"));
	PrefixQuery *query = _CLNEW PrefixQuery(t);
	IndexSearcher searcher(&directory);
	Hits *hits = searcher.search(query);
	CLUCENE_ASSERT(3 == hits->length()); // All documents in /Computers category and below
	_CLDELETE(query);
	_CLDELETE(t);
	_CLDELETE(hits);

	t = _CLNEW Term(_T("category"), _T("/Computers/Mac"));
	query = _CLNEW PrefixQuery(t);
	hits = searcher.search(query);
	CLUCENE_ASSERT(1 == hits->length()); // One in /Computers/Mac
	_CLDELETE(query);
	_CLDELETE(t);
	_CLDELETE(hits);
}

#ifndef NO_FUZZY_QUERY

class TestFuzzyQuery {
private:
	CuTest *tc;

	void addDoc(const TCHAR* text, IndexWriter* writer) {
		Document* doc = _CLNEW Document();
		doc->add(*_CLNEW Field(_T("field"), text, Field::STORE_YES | Field::INDEX_TOKENIZED));
		writer->addDocument(doc);
		_CLLDELETE(doc);
	}

	Hits* searchQuery(IndexSearcher* searcher, const TCHAR* field, const TCHAR* text,
		float_t minSimilarity=FuzzyQuery::defaultMinSimilarity, size_t prefixLen=0){

		Term* t = _CLNEW Term(field, text);
		FuzzyQuery* query = _CLNEW FuzzyQuery(t, minSimilarity, prefixLen);
		Hits* hits = searcher->search(query);
		_CLLDELETE(query);
		_CLLDECDELETE(t);
		return hits;
	}

	size_t getHitsLength(IndexSearcher* searcher, const TCHAR* field, const TCHAR* text,
		float_t minSimilarity=FuzzyQuery::defaultMinSimilarity, size_t prefixLen=0){

		Hits* hits = searchQuery(searcher, field, text, minSimilarity, prefixLen);
		size_t ret = hits->length();
		_CLLDELETE(hits);
		return ret;
	}
public:
	TestFuzzyQuery(CuTest *_tc):tc(_tc){
	}
	~TestFuzzyQuery(){
	}

	void testFuzziness() {
		RAMDirectory directory;
		WhitespaceAnalyzer a;
		IndexWriter writer(&directory, &a, true);
		addDoc(_T("aaaaa"), &writer);
		addDoc(_T("aaaab"), &writer);
		addDoc(_T("aaabb"), &writer);
		addDoc(_T("aabbb"), &writer);
		addDoc(_T("abbbb"), &writer);
		addDoc(_T("bbbbb"), &writer);
		addDoc(_T("ddddd"), &writer);
		writer.optimize();
		writer.close();
		IndexSearcher searcher(&directory);

		CLUCENE_ASSERT( getHitsLength(&searcher, _T("field"), _T("aaaaa")) == 3);

		// same with prefix
		CLUCENE_ASSERT( getHitsLength(&searcher, _T("field"), _T("aaaaa"),FuzzyQuery::defaultMinSimilarity,1) == 3);
		CLUCENE_ASSERT( getHitsLength(&searcher, _T("field"), _T("aaaaa"),FuzzyQuery::defaultMinSimilarity,2) == 3);
		CLUCENE_ASSERT( getHitsLength(&searcher, _T("field"), _T("aaaaa"),FuzzyQuery::defaultMinSimilarity,3) == 3);
		CLUCENE_ASSERT( getHitsLength(&searcher, _T("field"), _T("aaaaa"),FuzzyQuery::defaultMinSimilarity,4) == 2);
		CLUCENE_ASSERT( getHitsLength(&searcher, _T("field"), _T("aaaaa"),FuzzyQuery::defaultMinSimilarity,5) == 1);
		CLUCENE_ASSERT( getHitsLength(&searcher, _T("field"), _T("aaaaa"),FuzzyQuery::defaultMinSimilarity,6) == 0);

		// not similar enough:
		CuAssertTrue(tc, getHitsLength(&searcher, _T("field"), _T("xxxxx")) == 0);
		CuAssertTrue(tc, getHitsLength(&searcher, _T("field"), _T("aaccc")) == 0); // edit distance to "aaaaa" = 3

		// query identical to a word in the index:
		Hits* hits = searchQuery(&searcher, _T("field"), _T("aaaaa"));
		CLUCENE_ASSERT( hits->length() == 3);
		CuAssertStrEquals(tc, NULL, _T("aaaaa"), hits->doc(0).get(_T("field")));
		// default allows for up to two edits:
		CuAssertStrEquals(tc, NULL, _T("aaaab"), hits->doc(1).get(_T("field")));
		CuAssertStrEquals(tc, NULL, _T("aaabb"), hits->doc(2).get(_T("field")));
		_CLLDELETE(hits);

		// query similar to a word in the index:
		hits = searchQuery(&searcher, _T("field"), _T("aaaac"));
		CLUCENE_ASSERT( hits->length() == 3);
		CuAssertStrEquals(tc, NULL, _T("aaaaa"), hits->doc(0).get(_T("field")));
		CuAssertStrEquals(tc, NULL, _T("aaaab"), hits->doc(1).get(_T("field")));
		CuAssertStrEquals(tc, NULL, _T("aaabb"), hits->doc(2).get(_T("field")));
		_CLLDELETE(hits);

		// now with prefix
		hits = searchQuery(&searcher, _T("field"), _T("aaaac"), FuzzyQuery::defaultMinSimilarity, 1);
		CLUCENE_ASSERT( hits->length() == 3);
		CuAssertStrEquals(tc, NULL, _T("aaaaa"), hits->doc(0).get(_T("field")));
		CuAssertStrEquals(tc, NULL, _T("aaaab"), hits->doc(1).get(_T("field")));
		CuAssertStrEquals(tc, NULL, _T("aaabb"), hits->doc(2).get(_T("field")));
		_CLLDELETE(hits);

		hits = searchQuery(&searcher, _T("field"), _T("aaaac"), FuzzyQuery::defaultMinSimilarity, 2);
		CLUCENE_ASSERT( hits->length() == 3);
		CuAssertStrEquals(tc, NULL, _T("aaaaa"), hits->doc(0).get(_T("field")));
		CuAssertStrEquals(tc, NULL, _T("aaaab"), hits->doc(1).get(_T("field")));
		CuAssertStrEquals(tc, NULL, _T("aaabb"), hits->doc(2).get(_T("field")));
		_CLLDELETE(hits);

		hits = searchQuery(&searcher, _T("field"), _T("aaaac"), FuzzyQuery::defaultMinSimilarity, 3);
		CLUCENE_ASSERT( hits->length() == 3);
		CuAssertStrEquals(tc, NULL, _T("aaaaa"), hits->doc(0).get(_T("field")));
		CuAssertStrEquals(tc, NULL, _T("aaaab"), hits->doc(1).get(_T("field")));
		CuAssertStrEquals(tc, NULL, _T("aaabb"), hits->doc(2).get(_T("field")));
		_CLLDELETE(hits);

		hits = searchQuery(&searcher, _T("field"), _T("aaaac"), FuzzyQuery::defaultMinSimilarity, 4);
		CLUCENE_ASSERT( hits->length() == 2);
		CuAssertStrEquals(tc, NULL, _T("aaaaa"), hits->doc(0).get(_T("field")));
		CuAssertStrEquals(tc, NULL, _T("aaaab"), hits->doc(1).get(_T("field")));
		_CLLDELETE(hits);

		hits = searchQuery(&searcher, _T("field"), _T("aaaac"), FuzzyQuery::defaultMinSimilarity, 5);
		CLUCENE_ASSERT( hits->length() == 0);
		//CuAssertStrEquals(tc, NULL, _T("aaaaa"), hits->doc(0).get(_T("field")));
		_CLLDELETE(hits);


		hits = searchQuery(&searcher, _T("field"), _T("ddddX"));
		CLUCENE_ASSERT( hits->length() == 1);
		CuAssertStrEquals(tc, NULL, _T("ddddd"), hits->doc(0).get(_T("field")));
		_CLLDELETE(hits);

		// now with prefix
		hits = searchQuery(&searcher, _T("field"), _T("ddddX"), FuzzyQuery::defaultMinSimilarity, 1);
		CLUCENE_ASSERT( hits->length() == 1);
		CuAssertStrEquals(tc, NULL, _T("ddddd"), hits->doc(0).get(_T("field")));
		_CLLDELETE(hits);

		hits = searchQuery(&searcher, _T("field"), _T("ddddX"), FuzzyQuery::defaultMinSimilarity, 2);
		CLUCENE_ASSERT( hits->length() == 1);
		CuAssertStrEquals(tc, NULL, _T("ddddd"), hits->doc(0).get(_T("field")));
		_CLLDELETE(hits);

		hits = searchQuery(&searcher, _T("field"), _T("ddddX"), FuzzyQuery::defaultMinSimilarity, 3);
		CLUCENE_ASSERT( hits->length() == 1);
		CuAssertStrEquals(tc, NULL, _T("ddddd"), hits->doc(0).get(_T("field")));
		_CLLDELETE(hits);

		hits = searchQuery(&searcher, _T("field"), _T("ddddX"), FuzzyQuery::defaultMinSimilarity, 4);
		CLUCENE_ASSERT( hits->length() == 1);
		CuAssertStrEquals(tc, NULL, _T("ddddd"), hits->doc(0).get(_T("field")));
		_CLLDELETE(hits);

		hits = searchQuery(&searcher, _T("field"), _T("ddddX"), FuzzyQuery::defaultMinSimilarity, 5);
		CLUCENE_ASSERT( hits->length() == 0);
		_CLLDELETE(hits);

		// different field = no match:
		hits = searchQuery(&searcher, _T("anotherfield"), _T("ddddX"));
		CLUCENE_ASSERT( hits->length() == 0);
		_CLLDELETE(hits);

		searcher.close();
		directory.close();
	}

	/*
	void testFuzzinessLong() {
	RAMDirectory directory;
	IndexWriter writer = new IndexWriter(directory, new WhitespaceAnalyzer(), true);
	addDoc("aaaaaaa", writer);
	addDoc("segment", writer);
	writer.optimize();
	writer.close();
	IndexSearcher searcher = new IndexSearcher(directory);

	FuzzyQuery query;
	// not similar enough:
	query = new FuzzyQuery(new Term("field", "xxxxx"), FuzzyQuery.defaultMinSimilarity, 0);   
	Hits hits = searcher.search(query);
	assertEquals(0, hits.length());
	// edit distance to "aaaaaaa" = 3, this matches because the string is longer than
	// in testDefaultFuzziness so a bigger difference is allowed:
	query = new FuzzyQuery(new Term("field", "aaaaccc"), FuzzyQuery.defaultMinSimilarity, 0);   
	hits = searcher.search(query);
	assertEquals(1, hits.length());
	assertEquals(hits.doc(0).get("field"), ("aaaaaaa"));

	// now with prefix
	query = new FuzzyQuery(new Term("field", "aaaaccc"), FuzzyQuery.defaultMinSimilarity, 1);   
	hits = searcher.search(query);
	assertEquals(1, hits.length());
	assertEquals(hits.doc(0).get("field"), ("aaaaaaa"));
	query = new FuzzyQuery(new Term("field", "aaaaccc"), FuzzyQuery.defaultMinSimilarity, 4);   
	hits = searcher.search(query);
	assertEquals(1, hits.length());
	assertEquals(hits.doc(0).get("field"), ("aaaaaaa"));
	query = new FuzzyQuery(new Term("field", "aaaaccc"), FuzzyQuery.defaultMinSimilarity, 5);   
	hits = searcher.search(query);
	assertEquals(0, hits.length());

	// no match, more than half of the characters is wrong:
	query = new FuzzyQuery(new Term("field", "aaacccc"), FuzzyQuery.defaultMinSimilarity, 0);   
	hits = searcher.search(query);
	assertEquals(0, hits.length());

	// now with prefix
	query = new FuzzyQuery(new Term("field", "aaacccc"), FuzzyQuery.defaultMinSimilarity, 2);   
	hits = searcher.search(query);
	assertEquals(0, hits.length());

	// "student" and "stellent" are indeed similar to "segment" by default:
	query = new FuzzyQuery(new Term("field", "student"), FuzzyQuery.defaultMinSimilarity, 0);   
	hits = searcher.search(query);
	assertEquals(1, hits.length());
	query = new FuzzyQuery(new Term("field", "stellent"), FuzzyQuery.defaultMinSimilarity, 0);   
	hits = searcher.search(query);
	assertEquals(1, hits.length());

	// now with prefix
	query = new FuzzyQuery(new Term("field", "student"), FuzzyQuery.defaultMinSimilarity, 1);   
	hits = searcher.search(query);
	assertEquals(1, hits.length());
	query = new FuzzyQuery(new Term("field", "stellent"), FuzzyQuery.defaultMinSimilarity, 1);   
	hits = searcher.search(query);
	assertEquals(1, hits.length());
	query = new FuzzyQuery(new Term("field", "student"), FuzzyQuery.defaultMinSimilarity, 2);   
	hits = searcher.search(query);
	assertEquals(0, hits.length());
	query = new FuzzyQuery(new Term("field", "stellent"), FuzzyQuery.defaultMinSimilarity, 2);   
	hits = searcher.search(query);
	assertEquals(0, hits.length());

	// "student" doesn't match anymore thanks to increased minimum similarity:
	query = new FuzzyQuery(new Term("field", "student"), 0.6f, 0);   
	hits = searcher.search(query);
	assertEquals(0, hits.length());

	try {
	query = new FuzzyQuery(new Term("field", "student"), 1.1f);
	fail("Expected IllegalArgumentException");
	} catch (IllegalArgumentException e) {
	// expecting exception
	}
	try {
	query = new FuzzyQuery(new Term("field", "student"), -0.1f);
	fail("Expected IllegalArgumentException");
	} catch (IllegalArgumentException e) {
	// expecting exception
	}

	searcher.close();
	directory.close();
	}
	*/
};

void testFuzzyQuery(CuTest *tc){
	
	/// Run Java Lucene tests
	TestFuzzyQuery tester(tc);
	tester.testFuzziness();

	/// Legacy CLucene tests
	RAMDirectory ram;

	//---
	WhitespaceAnalyzer an;
	IndexWriter* writer = _CLNEW IndexWriter(&ram, &an, true);

	//---  
	Document *doc = 0;
	//****
	doc = _CLNEW Document();
	doc->add(*_CLNEW Field(_T("body"),_T("test"),Field::STORE_NO | Field::INDEX_TOKENIZED));
	writer->addDocument(doc);
	_CLDELETE(doc);
	//****
	doc = _CLNEW Document();
	doc->add(*_CLNEW Field(_T("body"),_T("home"),Field::STORE_NO | Field::INDEX_TOKENIZED));
	writer->addDocument(doc);
	_CLDELETE(doc);
	//****
	doc = _CLNEW Document();
	doc->add(*_CLNEW Field(_T("body"), _T("pc linux"),Field::STORE_NO | Field::INDEX_TOKENIZED));
	writer->addDocument(doc);
	_CLDELETE(doc);
	//****
	doc = _CLNEW Document();
	doc->add(*_CLNEW Field(_T("body"), _T("tested"),Field::STORE_NO | Field::INDEX_TOKENIZED));
	writer->addDocument(doc);
	_CLDELETE(doc);
	//****
	doc = _CLNEW Document();
	doc->add(*_CLNEW Field(_T("body"), _T("source"),Field::STORE_NO | Field::INDEX_TOKENIZED));
	writer->addDocument(doc);
	_CLDELETE(doc);

	//---
	writer->close();
	_CLDELETE(writer);

	//---
	IndexSearcher searcher (&ram);

	//---
	Term* term = _CLNEW Term(_T("body"), _T("test~"));
	Query* query = _CLNEW FuzzyQuery(term);
	Hits* result = searcher.search(query);

	CLUCENE_ASSERT(result && result->length() > 0);

	//---
	_CLDELETE(result);
	_CLDELETE(query);
	_CLDECDELETE(term);
	searcher.close();
	ram.close();
}
#else
	void _NO_FUZZY_QUERY(CuTest *tc){
		CuNotImpl(tc,_T("Fuzzy"));
	}
#endif

CuSuite *testqueries(void)
{
	CuSuite *suite = CuSuiteNew(_T("CLucene Queries Test"));

	SUITE_ADD_TEST(suite, testPrefixQuery);
	#ifndef NO_FUZZY_QUERY
		SUITE_ADD_TEST(suite, testFuzzyQuery);
	#else
		SUITE_ADD_TEST(suite, _NO_FUZZY_QUERY);
	#endif


	return suite; 
}
//EOF

