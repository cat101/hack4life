FROM ruby:3.1-alpine

# Set the working directory
WORKDIR /app

# Install system dependencies
RUN apk add --no-cache \
    build-base \
    git \
    nodejs \
    npm \
    linux-headers \
    libffi-dev

# Copy Gemfile
COPY Gemfile ./

# Install Ruby dependencies
RUN bundle config set --local path 'vendor/bundle' && \
    bundle install

# Copy the rest of the application
COPY . .

# Expose port 4000 for Jekyll server
EXPOSE 4000

# Set environment variables
ENV JEKYLL_ENV=production
ENV LC_ALL=C.UTF-8
ENV LANG=en_US.UTF-8
ENV LANGUAGE=en_US.UTF-8

# Create a non-root user
RUN addgroup -g 1000 jekyll && \
    adduser -u 1000 -G jekyll -s /bin/sh -D jekyll && \
    chown -R jekyll:jekyll /app

USER jekyll

# Default command - build the site
CMD ["bundle", "exec", "jekyll", "build", "--destination", "/app/_site"]

# Alternative commands can be:
# For development server: CMD ["bundle", "exec", "jekyll", "serve", "--host", "0.0.0.0", "--port", "4000"]
# For production build: CMD ["bundle", "exec", "jekyll", "build"]
