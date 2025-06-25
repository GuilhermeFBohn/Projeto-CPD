#include <iostream>
#include <fstream>
#include <list>
#include <cstring>
#include <filesystem>
#include "csv-parser\parser.hpp"
#include <iomanip>
#include <chrono>
#include <limits>
using namespace std;
using namespace aria::csv;
using namespace std::chrono;

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

// Receives the Trie root, a movie.title and movie.id. 
//Inserts a movie's title into the Trie
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

// Receives a Trie node and a list of movie.ids
// Inserts the ids of the Trie into the list
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

// Receives the Trie root, a string containing a prefix of a movie.title, and a list of movie.ids
// Looks for the prefix in the list of movie.ids
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

//------| MOVIE CLASS |------

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

//Hashtable Functions

//Turns a movie.id into a hash
int hashFunction(int id)
{
	return id % HASHSIZE;
}
//Receives a movie's information
//Transforms the information into a movie object and inserts it into the hash table
void insertMovie(int id, string title, string genres, int year)
{
	int i = hashFunction(id);
	Movie* newMovie = new Movie(id, title, genres, year);

	moviesHashTable[i].push_back(newMovie);
}
//Receives a moive.id
//Searches for the movie in the hash table
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

//Receives a path to a file
//Processes the information in the file and adds it to a hashtable of movies and a Trie of movie titles
void loadMovies(const string& path)
{
	std::ifstream file(path);
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
	}

}
//Receives a path to a file
//Processes the information, adds it to a hashtable of userRatings and and the rating's average
//to the corresponding movie
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
		cerr << "Arquivo esta corrompido ou ilegivel: " << path << endl;
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
//Outdated
void consultUser(int userId)
{
	int i = hashFunction(userId);
	cout << "Filmes avaliados pelo usuario" << userId << ":\n";
	for (auto& p : userRatingsTable[i])
	{
		if (Movie* m = findMovie(p.first))
		{
			cout << "- " << m->getTitle() << " (" << m->getYear() << ") Nota: " << p.second << endl;
		}
	}
}
//Turns a tag into a hash
int hashTag(const string& tag)
{
	unsigned long hash = 5301;
	for (char c : tag) hash = ((hash << 5) + hash) + c;
	return hash % TAG_HASHSIZE;
}
//Receives a tag and a movie.id
//Inserts it into a Hash table
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
//Receives a tag and returns a list of movies with the tag
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
//Receives a path to a file
//Processes the information and adds it into a hash table
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
//Outdated
void consultTag(const string& tag)
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
//Receives a string and lowercases the chars of the string
string stringtoLower(const string& str)
{
	string result = str;
	for (char& c : result)
	{
		c = tolower(c);
	}
	return result;
}
// receives a string inputed by the user, reads it and treats errors
int readInt(const string& prompt)
{
	int n;
	while (true)
	{
		cout << prompt;
		if (cin >> n)
		{
			cin.ignore(numeric_limits<streamsize>::max(), '\n');
			return n;
		}
		cin.clear();
		cin.ignore(numeric_limits<streamsize>::max(), '\n');
		cout << "Entrada invalida, tente novamente." << endl;
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
	string genreLower = stringtoLower(genre);
	list<Movie*> results;

	for (int i = 0; i < HASHSIZE; i++)
	{
		for (Movie* m : moviesHashTable[i])
		{
			string moviesGenresLower = stringtoLower(m->getGenres());
			if (m->getReviewCount() >= 1000 && moviesGenresLower.find(genreLower) != string::npos)
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
	bool alreadyExists;

	if (list1.empty() || list2.empty()) 
	{
		cout << "Uma das tags não retornou nenhum filme.\n";
		return;
	}


	for (int id1 : list1)
	{
		for (int id2 : list2)
		{
			if (id1 == id2)
			{
				alreadyExists = false;
				for (int existingId : intersec)
				{
					if (existingId == id1)
					{
						alreadyExists = true;
						break;
					}
				}
				if (!alreadyExists)
				{
					intersec.push_back(id1);
				}
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
	setlocale(LC_ALL, "");

	int id, sel, max;
	string prefix, tag1, tag2, genre, input;
	bool quit = false;
	auto start = high_resolution_clock::now();

	cout << "-----------------------------------------------------------------------------" << endl 
		 << "-------------------------------Movienator 3000-------------------------------" << endl
		 << "-----------------------------------------------------------------------------" << endl;

	cout << "Aguarde enquanto os dados sao carregados:" << endl;
	
	cout << "Carregando Filmes: ";
	loadMovies("../Data/dados-trabalho-completo/movies.csv");
	cout << "Concluido" << endl;
	cout << "Carregando reviews de usuarios: ";
	loadRatings("../Data/dados-trabalho-completo/ratings.csv");
	cout << "Concluido" << endl;
	cout << "Carregando tags de usuarios: ";
	loadTags("../Data/dados-trabalho-completo/tags.csv");
	cout << "Concluido" << endl;

	auto end = high_resolution_clock::now();
	auto duration = duration_cast<seconds>(end - start);
	cout << "Dados carregados em " << duration.count() << "segundos." << endl << endl;


	//------| Menu |------

	do
	{
		cout << "\n1) Pesquisar um filme" << endl;
		cout << "2) Pesquisar as reviews de um usuario" << endl;
		cout << "3) Pesquisar por genero" << endl;
		cout << "4) Pesquisar por tags (Formato: 'tag' || Ex: 'feel-good' 'predictable')" << endl;
		cout << "5) Sair" << endl;
		
		sel = readInt("Escolha: ");

		switch (sel)
		{
			case 1:
				cout << "Digite o nome do filme: ";
				getline(cin, prefix);
				prefixSearch(prefix);
				break;
			case 2:
				id = readInt("Digite o id do usuario: ");
				searchUserReviews(id);
				break;
			case 3:
				cout << "Digite o genero: ";
				getline(cin, genre);
				max = readInt("Digite o numero de filmes a exibir: ");
				searchbyGenres(genre, max);
				break;
			case 4:
				cout << "Digite a  primeira tag (entre aspas 'tag'): ";
				getline(cin, input);
				tag1 = input.substr(1, input.length() - 2);
				cout << "Digite a  segunda tag (entre aspas 'tag'): ";
				getline(cin, input);
				tag2 = input.substr(1, input.length() - 2);
				searchby2Tags(tag1, tag2);
				break;
			case 5:
				quit = true;
				break;
			default:
				cout << "Entrada invalida, tente novamente" << endl;
		}

	} while (!quit);

	return 0;
}

