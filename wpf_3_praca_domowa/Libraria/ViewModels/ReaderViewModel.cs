using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Windows;
using System.Windows.Documents;
using System.Windows.Media;
using Libraria.Models;

namespace Libraria.ViewModels
{
    public class ReaderViewModel : BaseViewModel
    {
        private const int TargetPageLength = 3500;

        private readonly Book _book;
        private readonly Action _goBack;
        private readonly List<string> _pages;
        private int _currentPage;
        private FlowDocument _currentDocument = new();

        public ReaderViewModel(Book book, int startPage, Action goBack)
        {
            _book = book;
            _goBack = goBack;
            _pages = LoadPages(book.FilePath);

            PreviousCommand = new RelayCommand(PreviousPage, () => CanGoPrevious);
            NextCommand = new RelayCommand(NextPage, () => CanGoNext);
            BackCommand = new RelayCommand(_goBack);

            if (_pages.Count > 0 && _book.PageCount != _pages.Count)
            {
                _book.PageCount = _pages.Count;
            }

            SetCurrentPage(Math.Clamp(startPage, 1, Math.Max(1, _pages.Count)));
        }

        public string Title => _book.Title;

        public string Author => _book.Author;

        public int CurrentPage
        {
            get => _currentPage;
            private set
            {
                if (SetProperty(ref _currentPage, value))
                {
                    _book.CurrentPage = value;
                    OnPropertyChanged(nameof(LocationText));
                    OnPropertyChanged(nameof(CanGoPrevious));
                    OnPropertyChanged(nameof(CanGoNext));
                    PreviousCommand.RaiseCanExecuteChanged();
                    NextCommand.RaiseCanExecuteChanged();
                }
            }
        }

        public int PageCount => _pages.Count;

        public string LocationText => PageCount == 0
            ? "No content"
            : $"Chapter {CurrentPage} of {PageCount}";

        public bool CanGoPrevious => CurrentPage > 1;

        public bool CanGoNext => CurrentPage < PageCount;

        public FlowDocument CurrentDocument
        {
            get => _currentDocument;
            private set => SetProperty(ref _currentDocument, value);
        }

        public RelayCommand PreviousCommand { get; }

        public RelayCommand NextCommand { get; }

        public RelayCommand BackCommand { get; }

        private void PreviousPage()
        {
            if (CanGoPrevious)
            {
                SetCurrentPage(CurrentPage - 1);
            }
        }

        private void NextPage()
        {
            if (CanGoNext)
            {
                SetCurrentPage(CurrentPage + 1);
            }
        }

        private void SetCurrentPage(int page)
        {
            CurrentPage = page;
            CurrentDocument = BuildDocument(_pages.Count == 0 ? "No readable Markdown content." : _pages[CurrentPage - 1]);
        }

        private static List<string> LoadPages(string filePath)
        {
            if (string.IsNullOrWhiteSpace(filePath) || !File.Exists(filePath))
            {
                return new List<string> { "The assigned Markdown file could not be found." };
            }

            var markdown = File.ReadAllText(filePath);
            return SplitMarkdown(markdown);
        }

        private static List<string> SplitMarkdown(string markdown)
        {
            var normalized = markdown.Replace("\r\n", "\n");
            var headingPages = SplitByHeadings(normalized);

            if (headingPages.Count > 1)
            {
                return headingPages;
            }

            return SplitByLength(normalized);
        }

        private static List<string> SplitByHeadings(string markdown)
        {
            var pages = new List<string>();
            var current = new List<string>();

            foreach (var line in markdown.Split('\n'))
            {
                if (line.StartsWith("# ", StringComparison.Ordinal) && current.Count > 0)
                {
                    pages.Add(string.Join(Environment.NewLine, current).Trim());
                    current.Clear();
                }

                current.Add(line);
            }

            if (current.Count > 0)
            {
                pages.Add(string.Join(Environment.NewLine, current).Trim());
            }

            return pages.Where(page => !string.IsNullOrWhiteSpace(page)).ToList();
        }

        private static List<string> SplitByLength(string markdown)
        {
            var pages = new List<string>();
            var paragraphs = markdown.Split(new[] { "\n\n" }, StringSplitOptions.None);
            var current = new List<string>();
            var currentLength = 0;

            foreach (var paragraph in paragraphs)
            {
                if (currentLength > 0 && currentLength + paragraph.Length > TargetPageLength)
                {
                    pages.Add(string.Join(Environment.NewLine + Environment.NewLine, current).Trim());
                    current.Clear();
                    currentLength = 0;
                }

                current.Add(paragraph);
                currentLength += paragraph.Length;
            }

            if (current.Count > 0)
            {
                pages.Add(string.Join(Environment.NewLine + Environment.NewLine, current).Trim());
            }

            return pages.Count == 0 ? new List<string> { "No readable Markdown content." } : pages;
        }

        private static FlowDocument BuildDocument(string markdown)
        {
            var document = new FlowDocument
            {
                PagePadding = new Thickness(36),
                FontFamily = new FontFamily("Segoe UI"),
                FontSize = 15,
                LineHeight = 24
            };

            var paragraphLines = new List<string>();

            foreach (var rawLine in markdown.Split(new[] { "\r\n", "\n" }, StringSplitOptions.None))
            {
                var line = rawLine.TrimEnd();

                if (string.IsNullOrWhiteSpace(line))
                {
                    AddParagraph(document, paragraphLines);
                    continue;
                }

                if (line.StartsWith("#", StringComparison.Ordinal))
                {
                    AddParagraph(document, paragraphLines);
                    AddHeading(document, line);
                    continue;
                }

                if (line.StartsWith("- ", StringComparison.Ordinal) || line.StartsWith("* ", StringComparison.Ordinal))
                {
                    AddParagraph(document, paragraphLines);
                    AddBullet(document, line[2..]);
                    continue;
                }

                paragraphLines.Add(line);
            }

            AddParagraph(document, paragraphLines);
            return document;
        }

        private static void AddHeading(FlowDocument document, string line)
        {
            var level = line.TakeWhile(character => character == '#').Count();
            var text = line[level..].Trim();
            var paragraph = new Paragraph(new Run(text))
            {
                FontWeight = FontWeights.SemiBold,
                FontSize = level switch
                {
                    1 => 28,
                    2 => 22,
                    _ => 18
                },
                Margin = new Thickness(0, 0, 0, 14)
            };

            document.Blocks.Add(paragraph);
        }

        private static void AddBullet(FlowDocument document, string text)
        {
            var paragraph = new Paragraph
            {
                Margin = new Thickness(18, 0, 0, 8)
            };

            paragraph.Inlines.Add(new Run("• ") { FontWeight = FontWeights.Bold });
            AddInlineMarkdown(paragraph, text);
            document.Blocks.Add(paragraph);
        }

        private static void AddParagraph(FlowDocument document, List<string> lines)
        {
            if (lines.Count == 0)
            {
                return;
            }

            var paragraph = new Paragraph
            {
                Margin = new Thickness(0, 0, 0, 12)
            };

            AddInlineMarkdown(paragraph, string.Join(" ", lines));
            document.Blocks.Add(paragraph);
            lines.Clear();
        }

        private static void AddInlineMarkdown(Paragraph paragraph, string text)
        {
            var parts = text.Split("**");

            for (var index = 0; index < parts.Length; index++)
            {
                var run = new Run(StripInlineMarkdown(parts[index]));

                if (index % 2 == 1)
                {
                    run.FontWeight = FontWeights.Bold;
                }

                paragraph.Inlines.Add(run);
            }
        }

        private static string StripInlineMarkdown(string text)
        {
            return text
                .Replace("**", string.Empty)
                .Replace("__", string.Empty)
                .Replace("*", string.Empty)
                .Replace("_", string.Empty)
                .Replace("`", string.Empty);
        }
    }
}
