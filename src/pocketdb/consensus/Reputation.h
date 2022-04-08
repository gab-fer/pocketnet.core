// Copyright (c) 2018-2022 The Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_REPUTATION_H
#define POCKETCONSENSUS_REPUTATION_H

#include "pocketdb/consensus/Base.h"

namespace PocketConsensus
{
    using namespace std;

    // ------------------------------------------
    // Consensus checkpoint at 0 block
    class ReputationConsensus : public BaseConsensus
    {
    protected:
        virtual int64_t GetMinLikers(int64_t registrationHeight)
        {
            return GetConsensusLimit(ConsensusLimit_threshold_likers_count);
        }
        
        virtual string SelectAddressScoreContent(shared_ptr<ScoreDataDto>& scoreData, bool lottery)
        {
            if (lottery)
                return scoreData->ScoreAddressHash;

            return scoreData->ContentAddressHash;
        }
        
        virtual bool AllowModifyReputationOverPost(shared_ptr<ScoreDataDto>& scoreData, bool lottery)
        {
            // Check user reputation
            if (!IsShark(SelectAddressScoreContent(scoreData, lottery), ConsensusLimit_threshold_reputation_score))
                return false;

            // Disable reputation increment if from one address to one address > 2 scores over day
            int64_t _max_scores_one_to_one = GetConsensusLimit(ConsensusLimit_scores_one_to_one);
            int64_t _scores_one_to_one_depth = GetConsensusLimit(ConsensusLimit_scores_one_to_one_depth);

            std::vector<int> values;
            if (lottery)
            {
                values.push_back(4);
                values.push_back(5);
            }
            else
            {
                values.push_back(1);
                values.push_back(2);
                values.push_back(3);
                values.push_back(4);
                values.push_back(5);
            }

            auto scores_one_to_one_count = PocketDb::ConsensusRepoInst.GetScoreContentCount(
                Height, scoreData, values, _scores_one_to_one_depth);

            if (scores_one_to_one_count >= _max_scores_one_to_one)
                return false;

            // All its Ok!
            return true;
        }

        virtual bool AllowModifyReputationOverComment(shared_ptr<ScoreDataDto>& scoreData, bool lottery)
        {
            // Check user reputation
            if (!IsShark(scoreData->ScoreAddressHash, ConsensusLimit_threshold_reputation_score))
                return false;

            // Disable reputation increment if from one address to one address > Limit::scores_one_to_one scores over Limit::scores_one_to_one_depth
            int64_t _max_scores_one_to_one = GetConsensusLimit(ConsensusLimit_scores_one_to_one_over_comment);
            int64_t _scores_one_to_one_depth = GetConsensusLimit(ConsensusLimit_scores_one_to_one_depth);

            std::vector<int> values;
            if (lottery)
            {
                values.push_back(1);
            }
            else
            {
                values.push_back(-1);
                values.push_back(1);
            }

            auto scores_one_to_one_count = PocketDb::ConsensusRepoInst.GetScoreCommentCount(
                Height, scoreData, values, _scores_one_to_one_depth);

            if (scores_one_to_one_count >= _max_scores_one_to_one)
                return false;

            // All its Ok!
            return true;
        }

    public:
        explicit ReputationConsensus(int height) : BaseConsensus(height) {}

        virtual bool IsShark(const string& address, ConsensusLimit limit = ConsensusLimit_threshold_reputation)
        {
            auto accountData = ConsensusRepoInst.GetAccountData(address);

            auto minReputation = GetConsensusLimit(limit);
            auto minLikersCount = GetMinLikers(accountData.RegistrationHeight);

            return accountData.Reputation >= minReputation && accountData.LikersCount >= minLikersCount;
        }

        virtual AccountMode GetAccountMode(int reputation, int64_t balance)
        {
            if (reputation >= GetConsensusLimit(ConsensusLimit_threshold_reputation) ||
                balance >= GetConsensusLimit(ConsensusLimit_threshold_balance))
                return AccountMode_Full;
            else
                return AccountMode_Trial;
        }

        virtual tuple<AccountMode, int, int64_t> GetAccountMode(string& address)
        {
            auto reputation = PocketDb::ConsensusRepoInst.GetUserReputation(address);
            auto balance = PocketDb::ConsensusRepoInst.GetUserBalance(address);

            return {GetAccountMode(reputation, balance), reputation, balance};
        }
        
        virtual bool AllowModifyReputation(shared_ptr<ScoreDataDto>& scoreData, bool lottery)
        {
            if (scoreData->ScoreType == TxType::ACTION_SCORE_CONTENT)
                return AllowModifyReputationOverPost(scoreData, lottery);

            if (scoreData->ScoreType == TxType::ACTION_SCORE_COMMENT)
                return AllowModifyReputationOverComment(scoreData, lottery);

            return false;
        }

        virtual bool AllowModifyOldPosts(int64_t scoreTime, int64_t contentTime, TxType contentType)
        {
            switch (contentType)
            {
                case CONTENT_POST:
                case CONTENT_VIDEO:
                case CONTENT_ARTICLE:
                    return (scoreTime - contentTime) < GetConsensusLimit(ConsensusLimit_scores_depth_modify_reputation);
                default:
                    return true;
            }
        }

        virtual void PrepareAccountLikers(map<int, vector<int>>& accountLikersSrc, map<int, vector<int>>& accountLikers)
        {
            for (const auto& account : accountLikersSrc)
            {
                for (const auto& likerId : account.second)
                {
                    if (!PocketDb::RatingsRepoInst.ExistsLiker(account.first, likerId, Height))
                    {
                        accountLikers[account.first].clear();
                        accountLikers[account.first].emplace_back(likerId);
                    }
                }
            }
        }
    
        virtual int GetScoreMultiplier(int scoreValue)
        {
            return (scoreValue - 3) * 10;
        }
    };

    // Consensus checkpoint at 151600 block
    class ReputationConsensus_checkpoint_151600 : public ReputationConsensus
    {
    public:
        explicit ReputationConsensus_checkpoint_151600(int height) : ReputationConsensus(height) {}
    protected:
        string SelectAddressScoreContent(shared_ptr<ScoreDataDto>& scoreData, bool lottery) override
        {
            return scoreData->ScoreAddressHash;
        }
    };

    // Consensus checkpoint at 1180000 block
    class ReputationConsensus_checkpoint_1180000 : public ReputationConsensus_checkpoint_151600
    {
    public:
        explicit ReputationConsensus_checkpoint_1180000(int height) : ReputationConsensus_checkpoint_151600(height) {}
    protected:
        int64_t GetMinLikers(int64_t registrationHeight) override
        {
            if (Height - registrationHeight > GetConsensusLimit(ConsensusLimit_threshold_low_likers_depth))
                return GetConsensusLimit(ConsensusLimit_threshold_low_likers_count);

            return GetConsensusLimit(ConsensusLimit_threshold_likers_count);
        }
    };

    // Consensus checkpoint at 1324655 block
    class ReputationConsensus_checkpoint_1324655 : public ReputationConsensus_checkpoint_1180000
    {
    public:
        explicit ReputationConsensus_checkpoint_1324655(int height) : ReputationConsensus_checkpoint_1180000(height) {}
        AccountMode GetAccountMode(int reputation, int64_t balance) override
        {
            if (reputation >= GetConsensusLimit(ConsensusLimit_threshold_reputation)
                || balance >= GetConsensusLimit(ConsensusLimit_threshold_balance))
            {
                if (balance >= GetConsensusLimit(ConsensusLimit_threshold_balance_pro))
                    return AccountMode_Pro;
                else
                    return AccountMode_Full;
            }
            else
            {
                return AccountMode_Trial;
            }
        }
    };

    // Consensus checkpoint at 1324655_2 block
    class ReputationConsensus_checkpoint_1324655_2 : public ReputationConsensus_checkpoint_1324655
    {
    public:
        explicit ReputationConsensus_checkpoint_1324655_2(int height) : ReputationConsensus_checkpoint_1324655(height) {}
        void PrepareAccountLikers(map<int, vector<int>>& accountLikersSrc, map<int, vector<int>>& accountLikers) override
        {
            for (const auto& account : accountLikersSrc)
                for (const auto& likerId : account.second)
                    if (!PocketDb::RatingsRepoInst.ExistsLiker(account.first, likerId, Height))
                        accountLikers[account.first].emplace_back(likerId);
        }
    };

    // Consensus checkpoint: reducing the impact on the reputation of scores 1,2 for content
    class ReputationConsensus_checkpoint_scores_reducing_impact : public ReputationConsensus_checkpoint_1324655_2
    {
    public:
        explicit ReputationConsensus_checkpoint_scores_reducing_impact(int height) : ReputationConsensus_checkpoint_1324655_2(height) {}
        int GetScoreMultiplier(int scoreValue) override
        {
            int multiplier = 10;
            if (scoreValue == 1 || scoreValue == 2)
                multiplier = 2;
            
            return (scoreValue - 3) * multiplier;
        }
    };

    //  Factory for select actual rules version
    class ReputationConsensusFactory
    {
    private:
        const vector<ConsensusCheckpoint<ReputationConsensus>> m_rules = {
            { 0,           -1, [](int height) { return make_shared<ReputationConsensus>(height); }},
            { 151600,      -1, [](int height) { return make_shared<ReputationConsensus_checkpoint_151600>(height); }},
            { 1180000,      0, [](int height) { return make_shared<ReputationConsensus_checkpoint_1180000>(height); }},
            { 1324655,  65000, [](int height) { return make_shared<ReputationConsensus_checkpoint_1324655>(height); }},
            { 1324655,  75000, [](int height) { return make_shared<ReputationConsensus_checkpoint_1324655_2>(height); }},
            { 9999999, 761000, [](int height) { return make_shared<ReputationConsensus_checkpoint_scores_reducing_impact>(height); }},
        };
    public:
        shared_ptr<ReputationConsensus> Instance(int height)
        {
            int m_height = (height > 0 ? height : 0);
            return (--upper_bound(m_rules.begin(), m_rules.end(), m_height,
                [&](int target, const ConsensusCheckpoint<ReputationConsensus>& itm)
                {
                    return target < itm.Height(Params().NetworkIDString());
                }
            ))->m_func(height);
        }
    };

    static ReputationConsensusFactory ReputationConsensusFactoryInst;
}

#endif // POCKETCONSENSUS_REPUTATION_H