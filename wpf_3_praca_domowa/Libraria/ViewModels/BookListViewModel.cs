using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Collections.Specialized;
using System.ComponentModel;
using System.Windows.Data;
using Libraria.Models;

namespace Libraria.ViewModels
{
    public class BookListViewModel : BaseViewModel
    {
        private readonly Action<Book> _openBook;
        private readonly HashSet<Book> _trackedBooks = new();
        private string _searchText = string.Empty;
        private StatusFilterOption _selectedStatusFilter;
        private RatingFilterOption _selectedRatingFilter;
        private SortOption _selectedSortOption;
        private readonly RelayCommand _clearFiltersCommand;

        public BookListViewModel(ObservableCollection<Book> books, Action<Book> openBook)
        {
            Books = books;
            _openBook = openBook;
            StatusFilters = new List<StatusFilterOption>
            {
                new("All", null),
                new("Unread", BookStatus.Unread),
                new("In Progress", BookStatus.InProgress),
                new("Read", BookStatus.Read)
            };
            RatingFilters = new List<RatingFilterOption>
            {
                new("All ratings", null),
                new("1+ stars", 1),
                new("2+ stars", 2),
                new("3+ stars", 3),
                new("4+ stars", 4),
                new("5 stars", 5)
            };
            SortOptions = new List<SortOption>
            {
                new("Title", nameof(Book.Title), ListSortDirection.Ascending),
                new("Author", nameof(Book.Author), ListSortDirection.Ascending),
                new("Rating", nameof(Book.Rating), ListSortDirection.Descending),
                new("Status", nameof(Book.Status), ListSortDirection.Ascending)
            };

            _selectedStatusFilter = StatusFilters[0];
            _selectedRatingFilter = RatingFilters[0];
            _selectedSortOption = SortOptions[0];

            FilteredBooks = CollectionViewSource.GetDefaultView(Books);
            FilteredBooks.Filter = FilterBook;

            Books.CollectionChanged += OnBooksCollectionChanged;
            foreach (var book in Books)
            {
                TrackBook(book);
            }

            _clearFiltersCommand = new RelayCommand(ClearFilters, () => HasActiveFilters);
            OpenBookCommand = new RelayCommand(OpenBook);
            ApplySorting();
        }

        public ObservableCollection<Book> Books { get; }

        public ICollectionView FilteredBooks { get; }

        public IReadOnlyList<StatusFilterOption> StatusFilters { get; }

        public IReadOnlyList<RatingFilterOption> RatingFilters { get; }

        public IReadOnlyList<SortOption> SortOptions { get; }

        public string SearchText
        {
            get => _searchText;
            set
            {
                if (SetProperty(ref _searchText, value ?? string.Empty))
                {
                    RefreshFilters();
                }
            }
        }

        public StatusFilterOption SelectedStatusFilter
        {
            get => _selectedStatusFilter;
            set
            {
                if (value is not null && SetProperty(ref _selectedStatusFilter, value))
                {
                    RefreshFilters();
                }
            }
        }

        public RatingFilterOption SelectedRatingFilter
        {
            get => _selectedRatingFilter;
            set
            {
                if (value is not null && SetProperty(ref _selectedRatingFilter, value))
                {
                    RefreshFilters();
                }
            }
        }

        public SortOption SelectedSortOption
        {
            get => _selectedSortOption;
            set
            {
                if (value is not null && SetProperty(ref _selectedSortOption, value))
                {
                    ApplySorting();
                    RefreshFilters();
                }
            }
        }

        public bool HasActiveFilters =>
            !string.IsNullOrWhiteSpace(SearchText)
            || SelectedStatusFilter.Status is not null
            || SelectedRatingFilter.MinimumRating is not null
            || SelectedSortOption != SortOptions[0];

        public RelayCommand ClearFiltersCommand => _clearFiltersCommand;

        public RelayCommand OpenBookCommand { get; }

        private void OpenBook(object? parameter)
        {
            if (parameter is Book book)
            {
                _openBook(book);
            }
        }

        private bool FilterBook(object item)
        {
            if (item is not Book book)
            {
                return false;
            }

            if (!MatchesSearch(book))
            {
                return false;
            }

            if (SelectedStatusFilter.Status is BookStatus status && book.Status != status)
            {
                return false;
            }

            if (SelectedRatingFilter.MinimumRating is int rating && book.Rating < rating)
            {
                return false;
            }

            return true;
        }

        private bool MatchesSearch(Book book)
        {
            if (string.IsNullOrWhiteSpace(SearchText))
            {
                return true;
            }

            return book.Title.Contains(SearchText, StringComparison.CurrentCultureIgnoreCase)
                || book.Author.Contains(SearchText, StringComparison.CurrentCultureIgnoreCase);
        }

        private void ApplySorting()
        {
            using (FilteredBooks.DeferRefresh())
            {
                FilteredBooks.SortDescriptions.Clear();
                FilteredBooks.SortDescriptions.Add(new SortDescription(
                    SelectedSortOption.PropertyName,
                    SelectedSortOption.Direction));
            }
        }

        private void ClearFilters()
        {
            SearchText = string.Empty;
            SelectedStatusFilter = StatusFilters[0];
            SelectedRatingFilter = RatingFilters[0];
            SelectedSortOption = SortOptions[0];
            RefreshFilters();
        }

        private void RefreshFilters()
        {
            FilteredBooks.Refresh();
            OnPropertyChanged(nameof(HasActiveFilters));
            System.Windows.Input.CommandManager.InvalidateRequerySuggested();
        }

        private void OnBooksCollectionChanged(object? sender, NotifyCollectionChangedEventArgs e)
        {
            if (e.Action == NotifyCollectionChangedAction.Reset)
            {
                foreach (var book in _trackedBooks)
                {
                    book.PropertyChanged -= OnBookPropertyChanged;
                }

                _trackedBooks.Clear();

                foreach (var book in Books)
                {
                    TrackBook(book);
                }

                RefreshFilters();
                return;
            }

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

            RefreshFilters();
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
            if (e.PropertyName is nameof(Book.Title)
                or nameof(Book.Author)
                or nameof(Book.Status)
                or nameof(Book.Rating))
            {
                RefreshFilters();
            }
        }
    }

    public sealed record StatusFilterOption(string Name, BookStatus? Status);

    public sealed record RatingFilterOption(string Name, int? MinimumRating);

    public sealed record SortOption(string Name, string PropertyName, ListSortDirection Direction);
}
