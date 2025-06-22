#include <iostream>
#include <fstream>
#include <list>
#include <cstring>
#include <filesystem>
#include "csv-parser\parser.hpp"
using namespace std;
using namespace aria::csv;

//---|TRIE Tree|---

struct TrieNode
{
	TrieNode* children[128];
	bool isEnd;
	int movieId;

	TrieNode()
	{
		for (int i = 0; i < 128;i++)
		{
			children[i] = nullptr;
		}
		isEnd = false;
		movieId = -1;
	}
};

TrieNode* trieRoot = new TrieNode();

void insertTitle(TrieNode* root, const string& title, int movieId)
{
	TrieNode* node = root;
	for (char ch : title)
	{
		int i = (unsigned char)ch;
		if (i < 0 || i >= 128)
		{
			continue;
		}
		if (node->children[i] == nullptr)
		{
			node->children[i] = new TrieNode();
		}
		node = node->children[i];
	}
	node->isEnd = true;
	node->movieId = movieId;
}

void listIds(TrieNode* node, list<int>& ids)
{
	if (node == nullptr)
	{
		return;
	}
	if (node->isEnd)
	{
		ids.push_back(node->movieId);
	}

	for (int i = 0; i < 128; i++)
	{
		if (node->children[i])
		{
			listIds(node->children[i], ids);
		}
	}

}

void findPrefix(TrieNode* root, const string& prefix, list<int>& ids)
{
	TrieNode* node = root;
	for (char ch : prefix)
	{
		int i = (unsigned char)ch;
		if (i < 0 || i >= 128 || node->children[i] == nullptr)
		{
			return;
		}
		node = node->children[i];
	}
	listIds(node, ids);
}



class Movie
{
	private:
		int id;
		string title;
		string genres;
		int year;
		int reviews = 0;
		float reviewsum = 0.0f;
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
		void addReview(float rating)
		{
			reviews++;
			reviewsum += rating;
		}

		float getAverage()
		{
			return reviews == 0 ? 0.0f : reviewsum / reviews;
		}
};

//---|Hashtable|---
const int HASHSIZE = 30000;
list<Movie*> moviesHashTable[HASHSIZE];
list<pair<int, float>> userRatingsTable[HASHSIZE];

const int TAG_HASHSIZE = 10007;
list<pair<string, list<int>>> tagsTable[TAG_HASHSIZE];

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
		insertTitle(trieRoot, title, id);
		//Teste
		cout << "Inserido: " << title << " (ID: " << id << ")\n";
	}

}

void loadRatings(const string& path)
{
	ifstream file(path);
	if (!file.is_open())
	{
		cerr << "Erro ao abrir o arquivo: " << path << endl;
		return;
	}
	file.peek();
	if (file.fail())
	{
		cerr << "Arquivo está corrompido ou ilegível: " << path << endl;
		return;
	}
	CsvParser parser(file);
	bool first = true;
	for (auto& row : parser)
	{
		if (first)
		{
			first = false;
			continue;
		}
		int userId = stoi(row[0]);
		int movieId = stoi(row[1]);
		float rating = stof(row[2]);
		userRatingsTable[hashFunction(userId)].push_back({movieId, rating});
		Movie* m = findMovie(movieId);
		if (m)
		{
			m->addReview(rating);
		}
	}
}

void consultaUsuario(int userId)
{
	int i = hashFunction(userId);
	cout << "Filmes avaliados pelo usuário" << userId << ":\n";
	for (auto& p : userRatingsTable[i])
	{
		if (Movie* m = findMovie(p.first))
		{
			cout << "- " << m->getTitle() << " (" << m->getYear() << ") Nota: " << p.second << endl;
		}
	}
}

int hashTag(const string& tag)
{
	unsigned long hash = 5301;
	for (char c : tag) hash = ((hash << 5) + hash) + c;
	return hash % TAG_HASHSIZE;
}

void insertTag(const string& tag, int movieId)
{
	int i = hashTag(tag);
	for (auto& p : tagsTable[i])
	{
		if (p.first == tag)
		{
			p.second.push_back(movieId);
			return;
		}
	}
	tagsTable[i].push_back({tag, {movieId}});
}

list<int> searchTag(const string& tag)
{
	int i = hashTag(tag);
	for (auto& p : tagsTable[i])
	{
		if (p.first == tag)
		{
			return p.second;
		}
	}
	return {};
}

void loadTags(const string& path)
{
	ifstream file(path);
	if (!file.is_open())
	{
		cerr << "Erro ao abrir o arquivo: " << path << endl;
		return;
	}
	file.peek();
	if (file.fail())
	{
		cerr << "Arquivo corrompido: " << path << endl;
		return;
	}
	CsvParser parser(file);
	bool first = true;
	for (auto& row : parser)
	{
		if (first)
		{
			first = false;
			continue;
		}
		int movieId = stoi(row[1]);
		string tag = row[2];
		insertTag(tag, movieId);
	}
}

void consultaTag(const string& tag)
{
	list<int> movies = searchTag(tag);
	if (movies.empty())
	{
		cout << "Nenhum filme com essa tag.\n";
		return;
	}
	cout << "Filmes com a tag '" << tag << "':\n";
	for (int id : movies)
	{
		if (Movie* m = findMovie(id))
		{
			cout << "- " << m->getTitle() << " (" << m->getYear() << ")\n";
		}
	}
}

int main() 
{
	int id;
	string prefix, tag;

	loadMovies("../Data/dados-trabalho-pequeno/movies.csv");
	loadRatings("../Data/dados-trabalho-pequeno/miniratings.csv");
	loadTags("../Data/dados-trabalho-pequeno/tags.csv");

	id = 1;
	if (Movie* m = findMovie(id))
	{
		cout << "Filme: " << m->getTitle() << " (" << m->getYear() << ")" << endl;
		cout << "Nota média: " << m->getAverage() << endl;
	}
	else
	{
		cout << "Filme com ID " << id << " não encontrado.\n";
	}

	cout << "Digite um prefixo para buscar filmes: ";
	getline(cin, prefix);

	list<int> resultados;
	findPrefix(trieRoot, prefix, resultados);

	if (resultados.empty())
	{
		cout << "Nenhum filme encontrado com esse prefixo" << endl;
	}
	else
	{
		cout << "Filmes encontrados: " << endl;
		for (int ID : resultados)
		{
			if (Movie* m = findMovie(ID))
			{
				cout << "- " << m->getTitle() << " (" << m->getYear() << ")\n";
			}
		}
	}

	cout << "\nDigite um ID de usuário para ver seus filmes avaliados: ";
	int uid;
	cin >> uid;
	cin.ignore();
	consultaUsuario(uid);

	cout << "\nDigite uma tag para buscar filmes: ";
	getline(cin, tag);
	consultaTag(tag);

	return 0;
}

