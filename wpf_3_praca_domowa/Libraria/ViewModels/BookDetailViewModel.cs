using System;
using System.Collections.ObjectModel;
using System.Collections.Specialized;
using System.ComponentModel;
using Libraria.Models;

namespace Libraria.ViewModels
{
    public class BookDetailViewModel : BaseViewModel
    {
        private readonly Action _goBack;
        private readonly Action<Book> _editBook;
        private readonly Action<Book, int> _openReader;
        private ObservableCollection<string>? _observedTags;

        public BookDetailViewModel(
            Book book,
            Action goBack,
            Action<Book> editBook,
            Action<Book, int> openReader)
        {
            Book = book;
            _goBack = goBack;
            _editBook = editBook;
            _openReader = openReader;

            BackCommand = new RelayCommand(_goBack);
            EditCommand = new RelayCommand(() => _editBook(Book));
            ReadFromStartCommand = new RelayCommand(ReadFromStart, () => HasBookFile);
            ContinueReadingCommand = new RelayCommand(ContinueReading, () => CanContinueReading);

            Book.PropertyChanged += OnBookPropertyChanged;
            SubscribeTags(Book.Tags);
        }

        public Book Book { get; }

        public string Title => Book.Title;

        public string Author => Book.Author;

        public string Description => Book.Description;

        public bool HasDescription => !string.IsNullOrWhiteSpace(Book.Description);

        public bool HasTags => Book.Tags.Count > 0;

        public bool HasPageCount => Book.PageCount > 0;

        public bool HasBookFile => !string.IsNullOrWhiteSpace(Book.FilePath);

        public bool CanContinueReading => HasBookFile && Book.CurrentPage > 1;

        public string ProgressText => HasPageCount
            ? $"Page {Math.Clamp(Book.CurrentPage, 1, Book.PageCount)} of {Book.PageCount}"
            : "Progress unavailable";

        public RelayCommand BackCommand { get; }

        public RelayCommand EditCommand { get; }

        public RelayCommand ReadFromStartCommand { get; }

        public RelayCommand ContinueReadingCommand { get; }

        private void ReadFromStart()
        {
            _openReader(Book, 1);
        }

        private void ContinueReading()
        {
            _openReader(Book, Math.Max(1, Book.CurrentPage));
        }

        private void OnBookPropertyChanged(object? sender, PropertyChangedEventArgs e)
        {
            switch (e.PropertyName)
            {
                case nameof(Book.Title):
                    OnPropertyChanged(nameof(Title));
                    break;
                case nameof(Book.Author):
                    OnPropertyChanged(nameof(Author));
                    break;
                case nameof(Book.Description):
                    OnPropertyChanged(nameof(Description));
                    OnPropertyChanged(nameof(HasDescription));
                    break;
                case nameof(Book.FilePath):
                    OnPropertyChanged(nameof(HasBookFile));
                    OnPropertyChanged(nameof(CanContinueReading));
                    ReadFromStartCommand.RaiseCanExecuteChanged();
                    ContinueReadingCommand.RaiseCanExecuteChanged();
                    break;
                case nameof(Book.CurrentPage):
                    OnPropertyChanged(nameof(ProgressText));
                    OnPropertyChanged(nameof(CanContinueReading));
                    ContinueReadingCommand.RaiseCanExecuteChanged();
                    break;
                case nameof(Book.PageCount):
                    OnPropertyChanged(nameof(HasPageCount));
                    OnPropertyChanged(nameof(ProgressText));
                    break;
                case nameof(Book.Tags):
                    SubscribeTags(Book.Tags);
                    break;
            }
        }

        private void SubscribeTags(ObservableCollection<string> tags)
        {
            if (_observedTags is not null)
            {
                _observedTags.CollectionChanged -= OnTagsChanged;
            }

            _observedTags = tags;
            _observedTags.CollectionChanged += OnTagsChanged;
            OnPropertyChanged(nameof(HasTags));
        }

        private void OnTagsChanged(object? sender, NotifyCollectionChangedEventArgs e)
        {
            OnPropertyChanged(nameof(HasTags));
        }
    }
}
