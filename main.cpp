#include <iostream>
#include <fstream>
#include <list>
#include <cstring>
#include <filesystem>
#include "csv-parser\parser.hpp"
#include <iomanip>
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
		int getReviewCount() const 
		{
			return reviews;
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

//-------------------------------------------
//---------------| PESQUISAS |---------------
//-------------------------------------------

void prefixSearch(const string& prefix)
{
	list<int> ids;
	findPrefix(trieRoot, prefix, ids);

	if (ids.empty())
	{
		cout << "Nenhum filme encontrado com esse prefixo.\n";
		return;
	}

	list<Movie*> movies;
	for (int id : ids)
	{
		Movie* m = findMovie(id);
		if (m)
		{
			movies.push_back(m);
		}
	}

	movies.sort([](Movie* a, Movie* b){
		return a->getAverage() > b->getAverage();
	});

	cout << left
		 << setw(8)  << "ID"
		 << setw(45) << "Title"
		 << setw(35) << "Genres"
		 << setw(6)  << "Year"
		 << setw(10) << "Rating"
		 << setw(6)  << "Count" << endl;

	cout << string(110, '-') << endl;

	for (Movie* m : movies)
	{
		cout << left
		     << setw(8)  << m->getId()
		     << setw(45) << m->getTitle().substr(0, 43)
		     << setw(35) << m->getGenres().substr(0, 33)
		     << setw(6)  << m->getYear()
		     << setw(10) << fixed << setprecision(6) << m->getAverage()
		     << setw(6)  << m->getReviewCount()
		     << endl;
	}
}

void searchUserReviews(int userId)
{
	int i = hashFunction(userId);
	list<pair<int, float>> reviews = userRatingsTable[i];

	list<pair<int, float>> userMovies;
	for (auto& p : reviews)
	{
		userMovies.push_back(p);
	}

	struct Output
	{
		int movieId;
		string title;
		string genres;
		int year;
		float globalAverage;
		int count;
		float userRating;
	};

	list<Output> results;

	for (auto& p : userMovies)
	{
		Movie* m = findMovie(p.first);
		if (m)
		{
			results.push_back({
				m->getId(),
				m->getTitle(),
				m->getGenres(),
				m->getYear(),
				m->getAverage(),
				m->getReviewCount(),
				p.second
			});
		}
	}

	results.sort([](const Output& a, const Output& b)
	{
		if (a.userRating != b.userRating)
			return a.userRating > b.userRating;
		return a.globalAverage > b.globalAverage;
	});

	cout << left << setw(8) << "ID" 
	     << setw(40) << "Título" 
	     << setw(30) << "Gêneros" 
	     << setw(6) << "Ano" 
	     << setw(14) << "MédiaGlobal" 
	     << setw(8) << "Count" 
	     << setw(8) << "Nota" << endl;

	int count = 0;
	for (auto& r : results) 
	{
		cout << setw(8) << r.movieId 
		     << setw(40) << r.title.substr(0, 39) 
		     << setw(30) << r.genres.substr(0, 29) 
		     << setw(6) << r.year 
		     << setw(14) << fixed << setprecision(6) << r.globalAverage 
		     << setw(8) << r.count 
		     << setw(8) << fixed << setprecision(1) << r.userRating 
		     << endl;

		if (++count >= 20) break;
	}
}

void searchbyGenres(const string& genre, int maxResults)
{
	list<Movie*> results;

	for (int i = 0; i < HASHSIZE; i++)
	{
		for (Movie* m : moviesHashTable[i])
		{
			if (m->getReviewCount() >= 1000 && m->getGenres().find(genre) != string::npos)
			{
				results.push_back(m);
			}
		}
	}

	if (results.empty())
	{
		cout << "Nenhum filme encontrado" << endl;
		return;
	}

	results.sort([](Movie * a, Movie * b)
	{
		return a->getAverage() > b->getAverage();
	});

	cout << left
	     << setw(8)  << "ID"
	     << setw(45) << "Title"
	     << setw(35) << "Genres"
	     << setw(6)  << "Year"
	     << setw(10) << "Rating"
	     << setw(6)  << "Count" << endl;

	cout << string(110, '-') << endl;

	int count = 0;
	for (Movie* m : results)
	{
		cout << left
		     << setw(8)  << m->getId()
		     << setw(45) << m->getTitle().substr(0, 43)
		     << setw(35) << m->getGenres().substr(0, 33)
		     << setw(6)  << m->getYear()
		     << setw(10) << fixed << setprecision(6) << m->getAverage()
		     << setw(6)  << m->getReviewCount()
		     << endl;

		if (++count >= maxResults)
		{
			break;
		}
	}
}

void searchby2Tags(const string& tag1, const string& tag2)
{
	list<int> list1 = searchTag(tag1);
	list<int> list2 = searchTag(tag2);

	list<int> intersec;

	for (int id1 : list1)
	{
		for (int id2 : list2)
		{
			if (id1 == id2)
			{
				intersec.push_back(id1);
				break;
			}
		}
	}

	if (intersec.empty())
	{
		cout << "Nenhum filme encontrado" << endl;
		return;
	}

	list<Movie*> results;
	for (int id : intersec)
	{
		if (Movie* m = findMovie(id))
		{
			results.push_back(m);
		}
	}

	results.sort([](Movie* a, Movie* b)
	{
		return a->getAverage() > b->getAverage();
	});

	cout << left
	     << setw(8)  << "ID"
	     << setw(45) << "Title"
	     << setw(35) << "Genres"
	     << setw(6)  << "Year"
	     << setw(10) << "Rating"
	     << setw(6)  << "Count" << endl;

	cout << string(110, '-') << endl;

	for (Movie* m : results)
	{
		cout << left
		     << setw(8)  << m->getId()
		     << setw(45) << m->getTitle().substr(0, 43)
		     << setw(35) << m->getGenres().substr(0, 33)
		     << setw(6)  << m->getYear()
		     << setw(10) << fixed << setprecision(6) << m->getAverage()
		     << setw(6)  << m->getReviewCount()
		     << endl;
	}
}

int main() 
{
	int id, sel, max;
	string prefix, tag1, tag2, genre, input;
	bool quit = false;

	loadMovies("../Data/dados-trabalho-pequeno/movies.csv");
	loadRatings("../Data/dados-trabalho-pequeno/miniratings.csv");
	loadTags("../Data/dados-trabalho-pequeno/tags.csv");

	//------| Menu |------

	do
	{
		cout << "1) Pesquisar um filme" << endl;
		cout << "2) Pesquisar as reviews de um usuário" << endl;
		cout << "3) Pesquisar por genêro" << endl;
		cout << "4) Pesquisar por tags" << endl;
		cout << "5) Sair" << endl;
		cin >> sel;
		cin.ignore();

		switch (sel)
		{
			case 1:
				cout << "Digite o nome do filme: ";
				getline(cin, prefix);
				prefixSearch(prefix);
				break;
			case 2:
				cout << "Digite o id do usuario: ";
				getline(cin, input);
				searchUserReviews(stoi(input));
				break;
			case 3:
				cout << "Digite o genero: ";
				getline(cin, genre);
				cout << "Digite o número de filmes a exibir: ";
				cin >> max;
				cin.ignore();
				searchbyGenres(genre, max);
				break;
			case 4:
				cout << "Digite a  primeira tag: ";
				getline(cin, input);
				tag1 = input.substr(1, input.length() - 2);
				cout << "Digite a  segunda tag: ";
				getline(cin, input);
				tag2 = input.substr(1, input.length() - 2);

				searchby2Tags(tag1, tag2);
				break;
			case 5:
				quit = true;
				break;
			default:
				cout << "Burro!" << endl;
		}

	} while (!quit);
	
	/*
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
	*/
	/*
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
	*/
	/*
	cout << "\nDigite um ID de usuário para ver seus filmes avaliados: ";
	int uid;
	cin >> uid;
	cin.ignore();
	consultaUsuario(uid);

	cout << "\nDigite uma tag para buscar filmes: ";
	getline(cin, tag);
	consultaTag(tag);

	return 0;
	*/
}

