#include <iostream>
#include <fstream>
#include <list>
#include <cstring>
#include <filesystem>
#include "csv-parser\parser.hpp"
using namespace std;
using namespace aria::csv;

/* Rascunho de Hash Table
class HashTable {
	private:
		static const int hashGroups = 10;
		list<pair<int, string>> table[hashGroups];
	public:
		bool isEmpty();
		int hashFunction(int key);
		void insertItem(int key, string value);
		void removeItem(int key);
		string searchTable(int key);
		void printTable();

}

bool HashTable::isEmpty(){
	int sum{};
	for (int i{}; i < hashGroups; i++)
	{
		sum += table[i].size();
	}

	if (!sum)
	{
		return true;
	}
	return false;
}

int HashTable::hashFunction(int key){
	return 0;
}

*/

class Movie
{
	private:
		int id;
		string title;
		string genres;
		int year;
		int reviews;
		float reviewsum;
	public:
		//Constructor
		Movie(int ID, string TITLE, string GENRES, int YEAR)
		{
			id = ID;
			title = TITLE;
			genres = GENRES;
			year = YEAR;
		}
		//Setters and getters
		void setId(int ID)
		{
			id = ID;
		}
		int getId()
		{
			return id;
		}
		void setTitle(string TITLE)
		{
			title = TITLE;
		}
		string getTitle()
		{
			return title;
		}
		void setGenres(string GENRES)
		{
			genres = GENRES;
		}
		string getGenres()
		{
			return genres;
		}
		void setYear(int YEAR)
		{
			year = YEAR;
		}
		int getYear()
		{
			return year;
		}
};

//---|Hashtable|---
const int HASHSIZE = 30000;
list<Movie*> moviesHashTable[HASHSIZE];

//Functions
int hashFunction(int id)
{
	return id % HASHSIZE;
}

void insertMovie(int id, string title, string genres, int year)
{
	int i = hashFunction(id);
	Movie* newMovie = new Movie(id, title, genres, year);

	moviesHashTable[i].push_back(newMovie);
}

Movie* findMovie(int id)
{
	int i = hashFunction(id);
	for (Movie* movie : moviesHashTable[i])
	{
		if (movie->getId() == id)
		{
			return movie;
		}
	}
	return nullptr;
}

void loadMovies(const string& path)
{
	//Teste
	std::ifstream file(path);
	if (!file.is_open()) {
    	std::cerr << "Erro ao abrir o arquivo: " << path << std::endl;
    	return;
	}

	file.peek(); // Força leitura

	if (file.fail()) {
    	std::cerr << "Arquivo está corrompido ou ilegível: " << path << std::endl;
    	return;
	}
	//Fim do teste

	//std::ifstream file(path);
	CsvParser parser(file);
	bool first = true;
	for (auto& row : parser)
	{
		if (first)
		{
			first = false;
			continue;
		}
		int id = stoi(row[0]);
		string title = row[1];
		string genres = row[2];
		int year = stoi(row[3]);

		insertMovie(id, title, genres, year);

	}

}

int main() 
{
	/* Rascunhos de codigo
	std::ifstream f("some_file.csv");
	CsvParser parser(f);

	std::cout << "Hello, world!";

	int array[20] = {};
	*/

	//Clearing hashtable
	/* Mais rascunho de codigo
	for (int i = 0; i < HASHSIZE; i++)
	{
    	moviesHashTable[i] = NULL;
	}
	*/

	cout << "Executando em: " << std::filesystem::current_path() << endl;



	int id;

	loadMovies("../Data/dados-trabalho-pequeno/movies.csv");


	//Acessing a movie
	id = 1;
	Movie* m = findMovie(id);
	if (m != nullptr)
	{
		cout << "Filme: " << m->getTitle() << " (" << m->getYear() << ")" << endl;
	}
	else
	{
		cout << "Filme com ID " << id << " não encontrado.\n";
	}

	return 0;
}

