#pragma once

// Abstract base: architecture + learned parameters, maps input -> output
class Model {
public:
    virtual ~Model() = default;
    virtual void infer() = 0; // input -> output
};

// Model subclass: transformer-based, autoregressive, trained on text corpora
class LLM : public Model {
public:
    void infer() override = 0; // still abstract at this level
};

// Concrete instance: specific weights, specific training run, specific release
class Sonnet5 : public LLM {
public:
    void infer() override {
        // fixed weights, deployable checkpoint
    }
};

// Harness: composition, not inheritance. Wraps a Model with runtime scaffolding:
// tool-calling, context/memory management, system prompt injection, I/O orchestration.
class Tool {
public:
    virtual ~Tool() = default;
    virtual void execute() = 0;
};

class Context {
public:
    void append(/* Input user_input */) {
        // accumulate conversation state (lives in the harness, not the model)
    }
};

class Harness {
    Model* model;                 // has-a, not is-a
    std::vector<Tool*> tools;
    Context context;

public:
    explicit Harness(Model* m) : model(m) {}

    void run(/* Input user_input */) {
        context.append(/* user_input */);
        model->infer(); // stateless call: all state lives in Harness::context
        // handle tool calls, loop if needed, update context
    }
};
