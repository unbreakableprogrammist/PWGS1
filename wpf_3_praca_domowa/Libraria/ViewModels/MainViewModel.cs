using System.Collections.ObjectModel;
using System.Collections.Specialized;
using System.ComponentModel;
using System.Globalization;
using System.IO;
using System.Linq;
using Libraria.Models;
using Libraria.Services;

namespace Libraria.ViewModels
{
    public class MainViewModel : BaseViewModel
    {
        private readonly RepositoryService _repositoryService; // obiekt odopowiada za ladowanie i zapisywanie danych do repozytoriun
        private readonly IBookEditDialogService _bookEditDialogService; // obiekt ktory odpowiada za pokazanie okienka od edycji ksiazki
        private readonly WelcomeViewModel _welcomeViewModel;  // obiekt ktory ogarnia to okienko poczatkowe 
        private readonly HashSet<Book> _trackedBooks = new(); // ktore ksiazki obecnie sledzimy 
        private object? _currentPage;
        private Book? _selectedBook;
        private string _statusMessage = "Open or create a repository to start.";

        // konstruktory 
        public MainViewModel()
            : this(new RepositoryService(), new BookEditDialogService())
        {
        }

        public MainViewModel(RepositoryService repositoryService)
            : this(repositoryService, new BookEditDialogService())
        {
        }

        public MainViewModel(RepositoryService repositoryService, IBookEditDialogService bookEditDialogService)
        {
            _repositoryService = repositoryService;
            _bookEditDialogService = bookEditDialogService;
            _repositoryService.Books.CollectionChanged += OnBooksChanged;
            _welcomeViewModel = new WelcomeViewModel(_repositoryService, OnRepositoryReady);

            CurrentPage = _welcomeViewModel;

            // przypisujemy komendy 
            CreateRepositoryCommand = _welcomeViewModel.CreateRepositoryCommand;
            OpenRepositoryCommand = _welcomeViewModel.OpenRepositoryCommand;
            AddBookCommand = new RelayCommand(AddBook, () => IsRepositoryOpen);
            EditBookCommand = new RelayCommand(EditSelectedBook, () => SelectedBook is not null);
            DeleteBookCommand = new RelayCommand(DeleteSelectedBook, () => SelectedBook is not null);
        }

        public object? CurrentPage
        {
            get => _currentPage;
            set => SetProperty(ref _currentPage, value);
        }

        public ObservableCollection<Book> Books => _repositoryService.Books;

        public Book? SelectedBook
        {
            get => _selectedBook;
            set
            {
                if (SetProperty(ref _selectedBook, value))
                {
                    CommandManagerRefresh();
                }
            }
        }

        public string StatusMessage
        {
            get => _statusMessage;
            set => SetProperty(ref _statusMessage, value);
        }

        public int BookCount => Books.Count;

        public int ReadBookCount => Books.Count(book => book.Status == BookStatus.Read);

        public int InProgressBookCount => Books.Count(book => book.Status == BookStatus.InProgress);

        public string AverageRating
        {
            get
            {
                var ratedBooks = Books.Where(book => book.Rating > 0).ToList();
                return ratedBooks.Count == 0
                    ? "—"
                    : $"{ratedBooks.Average(book => book.Rating).ToString("0.0", CultureInfo.CurrentCulture)} ★";
            }
        }

        public string TopGenre => Books
            .Select(book => book.Genre.Trim())
            .Where(genre => !string.IsNullOrWhiteSpace(genre))
            .GroupBy(genre => genre)
            .OrderByDescending(group => group.Count())
            .ThenBy(group => group.Key)
            .Select(group => group.Key)
            .FirstOrDefault() ?? "—";

        public bool IsRepositoryOpen => _repositoryService.IsRepositoryOpen;

        public string RepositoryName => _repositoryService.RepositoryPath is null
            ? "No repository"
            : Path.GetFileName(_repositoryService.RepositoryPath);

        public RelayCommand CreateRepositoryCommand { get; }

        public RelayCommand OpenRepositoryCommand { get; }

        public RelayCommand AddBookCommand { get; }

        public RelayCommand EditBookCommand { get; }

        public RelayCommand DeleteBookCommand { get; }

        private void OnRepositoryReady(string statusMessage)
        {
            NavigateToBookList();
            StatusMessage = statusMessage;
            RefreshRepositoryState();
        }

        private void NavigateToBookList()
        {
            SelectedBook = null;
            CurrentPage = new BookListViewModel(Books, NavigateToBookDetail);
        }

        private void NavigateToBookDetail(Book book)
        {
            SelectedBook = book;
            CurrentPage = new BookDetailViewModel(
                book,
                NavigateToBookList,
                EditBook,
                OpenReader);
        }

        private void OpenReader(Book book, int startPage)
        {
            SelectedBook = book;
            CurrentPage = new ReaderViewModel(
                book,
                startPage,
                () => NavigateToBookDetail(book));
            StatusMessage = "Reader opened.";
        }

        private void AddBook()
        {
            var book = new Book();
            var viewModel = new BookEditViewModel(book, true);

            if (!_bookEditDialogService.ShowDialog(viewModel))
            {
                return;
            }

            Books.Add(book);
            SelectedBook = book;
            StatusMessage = "Book added.";
        }

        private void EditSelectedBook()
        {
            if (SelectedBook is null)
            {
                return;
            }

            EditBook(SelectedBook);
        }

        private void EditBook(Book book)
        {
            var viewModel = new BookEditViewModel(book);

            if (_bookEditDialogService.ShowDialog(viewModel))
            {
                StatusMessage = "Book updated.";
                RefreshStatistics();
            }
        }
        // usuwamy ksiazke 
        private void DeleteSelectedBook()
        {
            if (SelectedBook is null)
            {
                return;
            }

            var book = SelectedBook;
            SelectedBook = null;
            Books.Remove(book);
            if (CurrentPage is not BookListViewModel && CurrentPage is not WelcomeViewModel)
            {
                NavigateToBookList();
            }

            StatusMessage = "Book removed.";
        }

        private void OnBooksChanged(object? sender, NotifyCollectionChangedEventArgs e)
        {
            if (e.Action == NotifyCollectionChangedAction.Reset)
            {
                foreach (var book in _trackedBooks.ToList())
                {
                    UntrackBook(book);
                }

                foreach (var book in Books)
                {
                    TrackBook(book);
                }
            }
            else
            {
                if (e.OldItems is not null)
                {
                    foreach (Book book in e.OldItems)
                    {
                        UntrackBook(book);
                    }
                }

                if (e.NewItems is not null)
                {
                    foreach (Book book in e.NewItems)
                    {
                        TrackBook(book);
                    }
                }
            }

            RefreshStatistics();
            CommandManagerRefresh();
        }

        private void RefreshRepositoryState()
        {
            RefreshStatistics();
            OnPropertyChanged(nameof(IsRepositoryOpen));
            OnPropertyChanged(nameof(RepositoryName));
            CommandManagerRefresh();
        }

        private void TrackBook(Book book)
        {
            if (_trackedBooks.Add(book))
            {
                book.PropertyChanged += OnBookPropertyChanged;
            }
        }

        private void UntrackBook(Book book)
        {
            if (_trackedBooks.Remove(book))
            {
                book.PropertyChanged -= OnBookPropertyChanged;
            }
        }

        private void OnBookPropertyChanged(object? sender, PropertyChangedEventArgs e)
        {
            if (e.PropertyName is nameof(Book.Status)
                or nameof(Book.Rating)
                or nameof(Book.Genre))
            {
                RefreshStatistics();
            }
        }

        private void RefreshStatistics()
        {
            OnPropertyChanged(nameof(BookCount));
            OnPropertyChanged(nameof(ReadBookCount));
            OnPropertyChanged(nameof(InProgressBookCount));
            OnPropertyChanged(nameof(AverageRating));
            OnPropertyChanged(nameof(TopGenre));
        }

        private static void CommandManagerRefresh()
        {
            System.Windows.Input.CommandManager.InvalidateRequerySuggested();
        }
    }
}
