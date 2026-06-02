using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Collections.Specialized;
using System.IO;
using System.Linq;
using Libraria.Models;
using Newtonsoft.Json;

namespace Libraria.Services
{
    public class RepositoryService
    {
        private readonly HashSet<Book> _trackedBooks = new();
        private readonly Dictionary<Book, TagSubscription> _tagSubscriptions = new();
        private bool _suspendAutoSave;

        public RepositoryService()
        {
            Books.CollectionChanged += OnBooksCollectionChanged;
        }

        public ObservableCollection<Book> Books { get; } = new();

        public string? RepositoryPath { get; private set; }

        public bool IsRepositoryOpen => !string.IsNullOrWhiteSpace(RepositoryPath);

        public void Create(string filePath)
        {
            ArgumentException.ThrowIfNullOrWhiteSpace(filePath);

            RepositoryPath = filePath;
            ReplaceBooks(Array.Empty<Book>());
            Save();
        }

        public void Open(string filePath)
        {
            ArgumentException.ThrowIfNullOrWhiteSpace(filePath);

            var books = Load(filePath);
            RepositoryPath = filePath;
            ReplaceBooks(books);
        }

        public List<Book> Load(string filePath)
        {
            ArgumentException.ThrowIfNullOrWhiteSpace(filePath);

            if (!File.Exists(filePath))
            {
                throw new FileNotFoundException("Repository file was not found.", filePath);
            }

            var json = File.ReadAllText(filePath);
            var books = JsonConvert.DeserializeObject<List<Book>>(json) ?? new List<Book>();

            foreach (var book in books)
            {
                NormalizeBook(book);
            }

            return books;
        }

        public void Save(string filePath, List<Book> books)
        {
            ArgumentException.ThrowIfNullOrWhiteSpace(filePath);
            ArgumentNullException.ThrowIfNull(books);

            var directory = Path.GetDirectoryName(filePath);
            if (!string.IsNullOrWhiteSpace(directory))
            {
                Directory.CreateDirectory(directory);
            }

            var json = JsonConvert.SerializeObject(books, Formatting.Indented);
            File.WriteAllText(filePath, json);
        }

        public void Save()
        {
            if (!IsRepositoryOpen)
            {
                return;
            }

            Save(RepositoryPath!, Books.ToList());
        }

        private void ReplaceBooks(IEnumerable<Book> books)
        {
            _suspendAutoSave = true;

            try
            {
                foreach (var book in _trackedBooks.ToList())
                {
                    UntrackBook(book);
                }

                Books.Clear();

                foreach (var book in books)
                {
                    NormalizeBook(book);
                    Books.Add(book);
                }
            }
            finally
            {
                _suspendAutoSave = false;
            }
        }

        private void OnBooksCollectionChanged(object? sender, NotifyCollectionChangedEventArgs e)
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

                AutoSave();
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

            AutoSave();
        }

        private void TrackBook(Book book)
        {
            if (!_trackedBooks.Add(book))
            {
                return;
            }

            book.PropertyChanged += OnBookPropertyChanged;
            TrackTags(book);
        }

        private void UntrackBook(Book book)
        {
            if (!_trackedBooks.Remove(book))
            {
                return;
            }

            book.PropertyChanged -= OnBookPropertyChanged;
            UntrackTags(book);
        }

        private void OnBookPropertyChanged(object? sender, System.ComponentModel.PropertyChangedEventArgs e)
        {
            if (sender is Book book && e.PropertyName == nameof(Book.Tags))
            {
                TrackTags(book);
            }

            AutoSave();
        }

        private void TrackTags(Book book)
        {
            UntrackTags(book);

            NotifyCollectionChangedEventHandler handler = (_, _) => AutoSave();
            book.Tags.CollectionChanged += handler;
            _tagSubscriptions[book] = new TagSubscription(book.Tags, handler);
        }

        private void UntrackTags(Book book)
        {
            if (!_tagSubscriptions.Remove(book, out var subscription))
            {
                return;
            }

            subscription.Tags.CollectionChanged -= subscription.Handler;
        }

        private void AutoSave()
        {
            if (_suspendAutoSave || !IsRepositoryOpen)
            {
                return;
            }

            Save();
        }

        private static void NormalizeBook(Book book)
        {
            if (book.Id == Guid.Empty)
            {
                book.Id = Guid.NewGuid();
            }

            book.Tags ??= new ObservableCollection<string>();
            book.CoverImage ??= Array.Empty<byte>();

            if (book.CurrentPage < 1)
            {
                book.CurrentPage = 1;
            }
        }

        private sealed record TagSubscription(
            ObservableCollection<string> Tags,
            NotifyCollectionChangedEventHandler Handler);
    }
}
